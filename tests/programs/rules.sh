#!/bin/sh
# Probe for TST.referee.2-8@1: in a throwaway copy, petition 0001 (hanoi), which
# passes rules 2 to 8, is broken seven ways, one rule at a time, and
# `jig/aios rules` must name that rule against petition 0001 each time.
set -u
cd "$(dirname "$0")/../.."
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
mkdir "$t/base"; tar --exclude=./out --exclude=./.git -cf - . | tar -xf - -C "$t/base"
(cd "$t/base" && sh jig/aios rules > /dev/null 2>&1; grep -q '^FAIL  rule [2-8]  petition 0001' tests/referee-results.txt) && { echo "petition 0001 already fails a rule; the probe has nothing to break"; exit 1; }
expect() {	# N: rule N must FAIL for petition 0001 in the copy at $t/c
	(cd "$t/c" && sh jig/aios rules > /dev/null 2>&1)
	grep -q "^FAIL  rule $1  petition 0001" "$t/c/tests/referee-results.txt" || { echo "rule $1 broken on purpose and not named: $(grep "rule $1  petition 0001" "$t/c/tests/referee-results.txt")"; exit 1; }
}
fresh() { rm -rf "$t/c"; cp -r "$t/base" "$t/c"; }
pet="petitions/0001.toml"; cat="index/part/hanoi@1.toml"
fresh; sed -i '0,/^reason/s/^reason *= *".*"/reason = "SPEC.editor.1@1"/' "$t/c/$pet"; expect 2
fresh; printf 'code       = "SPEC.hanoi.9@1"\nstatus     = "draft"\nadmitted   = ""\nretired_by = ""\ncomponent  = "PART.hanoi@1"\nclause     = "A clause with no test."\nauthor     = "KEY.chris"\n' > "$t/c/specs/hanoi.9@1.toml"; expect 3
fresh; sed -i 's/^requires *= *\[\(.*\)\]/requires      = [\1, "CAP.dev.disk@1"]/' "$t/c/$cat"; expect 4
fresh; printf '\n[[capability]]\ncode   = "CAP.dev.disk@1"\nreason = "SPEC.hanoi.1@1"\nscope  = ""\n' >> "$t/c/$pet"; expect 5
fresh; sed -i 's/^depends *= *\[\(.*\)\]/depends       = [\1, "PART.disk.ata-pio@1"]/' "$t/c/$cat"; expect 6
fresh; sed -i '0,/^admitted = false/s/^admitted = false/admitted = true/' "$t/c/$pet"; expect 7
fresh; printf '\n[[sensor]]\ncode = "SNS.hanoi.nothing@1"\n' >> "$t/c/$pet"; expect 8
echo "petition 0001 broken seven ways, one rule at a time; the referee named every rule"
