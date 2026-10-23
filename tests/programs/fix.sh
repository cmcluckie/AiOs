#!/bin/sh
# Probe for TST.fix.judged@1: jig/fix in a throwaway copy of the repo that has a
# made-up part, pool/PART.fixprobe@1/f.c ("broken"), and two made-up tests, also
# only in the copy: fixprobe.red, green when f.c says "fixed"; fixprobe.other,
# red when f.c says "breaks-other".  The model is saved replies (MODEL_FAKE),
# and the suite the judge runs is the two made-up tests (FIX_SUITE), so nothing
# boots.  Green when:
#   a green test is refused before any call ("nothing to fix"), and no run is kept
#   the model's look at f.c is answered with what f.c says, and a look outside
#     the repo is refused
#   a fix that turns the red test green with the rest still green is recorded as
#     pass, with its two changed lines and the tokens of every call
#   a fix that turns the red test green but another test red is a fail
#   a test that was red before the fix and is still red after (fixprobe.always,
#     another job) does not count against it
#   a fix handed in for a file outside pool/ is refused, and nothing is judged
#   the copy the model worked from never changes: f.c still says "broken"
# Exit 0 green, 1 red, 2 where jig/test cannot run (no QEMU) or there is no jq.
set -u
cd "$(dirname "$0")/../.."
fail() { echo "$*"; exit 1; }
command -v qemu-system-x86_64 > /dev/null || { echo "no QEMU here, so jig/test does not run"; exit 2; }
command -v jq > /dev/null || { echo "no jq here"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
mkdir "$t/r"; tar --exclude=./.git --exclude=./out --exclude=./runs -cf - . | tar -xf - -C "$t/r"
r="$t/r"; mkdir -p "$r/pool/PART.fixprobe@1"; printf 'broken\n' > "$r/pool/PART.fixprobe@1/f.c"
printf '#!/bin/sh\ngrep -qx fixed pool/PART.fixprobe@1/f.c && { echo fixed; exit 0; }\necho "f.c says $(head -1 pool/PART.fixprobe@1/f.c)"; exit 1\n' > "$r/tests/programs/fixprobe-red.sh"
printf '#!/bin/sh\ngrep -q breaks-other pool/PART.fixprobe@1/f.c && { echo "broken by the fix"; exit 1; }\necho fine; exit 0\n' > "$r/tests/programs/fixprobe-other.sh"
printf '#!/bin/sh\necho "red before any fix, and after"; exit 1\n' > "$r/tests/programs/fixprobe-always.sh"
for n in red other always; do
	printf 'code       = "TST.fixprobe.%s@1"\nstatus     = "draft"\nadmitted   = ""\nretired_by = ""\nbinds      = "SPEC.fixprobe.1@1"\nred_when   = "f.c is not fixed"\ntimeout_ms = 10000\nisolates   = "PART.fixprobe@1"\nrunner     = "jig/test cmd sh tests/programs/fixprobe-%s.sh"\nmutations  = 0\n' "$n" "$n" > "$r/tests/fixprobe.$n@1.toml"
done
printf 'code       = "SPEC.fixprobe.1@1"\nstatus     = "draft"\nadmitted   = ""\nretired_by = ""\ncomponent  = "PART.fixprobe@1"\nclause     = "f.c says fixed."\nauthor     = "KEY.chris"\n' > "$r/specs/fixprobe.1@1.toml"
reply() {	# reply FILE TOOL INPUT_JSON: a saved Converse reply that calls one tool
	printf '{"output":{"message":{"role":"assistant","content":[{"toolUse":{"toolUseId":"u-%s","name":"%s","input":%s}}]}},"stopReason":"tool_use","usage":{"inputTokens":100,"outputTokens":20,"cacheReadInputTokens":1000}}\n' "$(basename "$1" .json)" "$2" "$3" > "$1"
}
fakes() {	# fakes DIR CONTENT [PATH]: look at f.c, look outside, read lines 2 to 2 given as
	# two numbers, find a word in one folder, then hand in CONTENT
	mkdir -p "$1"
	reply "$1/1.json" look '{"command":"read","args":["pool/PART.fixprobe@1/f.c"]}'
	reply "$1/2.json" look '{"command":"read","args":["../../etc/passwd"]}'
	reply "$1/3.json" look '{"command":"read","args":["tests/programs/fixprobe-red.sh","2","2"]}'
	reply "$1/4.json" look '{"command":"find","args":["broken","pool/PART.fixprobe@1"]}'
	reply "$1/5.json" submit_fix "$(jq -cn --arg p "${3:-pool/PART.fixprobe@1/f.c}" --arg c "$2" '{path:$p,content:$c}')"
}
export PRICES="$r/tests/fixtures/model/prices.toml" MODEL_NAME=test:model RUNS="$t/runs" BUDGET="$r/tests/fixtures/exam/budget.toml" FIX_SUITE="fixprobe." FIX_OUT="$t/out"
run() { (cd "$r" && sh jig/fix "$@") > "$t/said" 2>&1; }

fakes "$t/f-green" "fixed
"
MODEL_FAKE="$t/f-green" run fixprobe.other@1; rc=$?
[ $rc -eq 1 ] && grep -q "nothing to fix" "$t/said" || fail "a green test was not refused as nothing to fix ($rc): $(tail -1 "$t/said")"
[ ! -e "$t/f-green/.n" ] && [ ! -d "$t/runs" -o -z "$(ls "$t/runs" 2> /dev/null)" ] || fail "a green test cost a call or left a run"

MODEL_FAKE="$t/f-green" run fixprobe.red@1 || fail "a good fix was not judged a pass: $(tail -2 "$t/said" | tr '\n' ' ')"
in=$(ls -d "$t/runs"/*/ | head -1)
grep -q '^result *= "pass"' "$in/run.toml" && grep -q '^lines *= 2$' "$in/run.toml" || fail "the pass was not recorded with its 2 changed lines: $(grep -E '^(result|lines)' "$in/run.toml" | tr '\n' ' ')"
grep -q '^input_tokens *= 500$' "$in/run.toml" && grep -q '^output_tokens *= 100$' "$in/run.toml" && grep -q '^cache_read_tokens *= 5000$' "$in/run.toml" || fail "the tokens of every call were not counted: $(grep '_tokens' "$in/run.toml" | tr '\n' ' ')"
grep -q '"text": *"1\\tbroken' "$in/input.txt" || fail "the model's look at f.c was not answered with what f.c says"
grep -q 'outside the repo' "$in/input.txt" || fail "a look outside the repo was not refused"
grep -q '"text": *"2\\tgrep -qx fixed' "$in/input.txt" && ! grep -q '"text": *"1\\t#!/bin/sh\\n2' "$in/input.txt" || fail "a read of lines 2 to 2, given as two numbers, did not give line 2 alone"
grep -q 'pool/PART.fixprobe@1/f.c:1:broken' "$in/input.txt" && ! grep -q 'tests/programs/fix.sh:[0-9]*:' "$in/input.txt" || fail "a find in one folder was not kept to that folder"
grep -q '5000 read from the cache' "$t/said" || fail "the summary line leaves out the tokens read from the cache: $(tail -1 "$t/said")"
grep -qx broken "$r/pool/PART.fixprobe@1/f.c" || fail "the copy the model worked from was changed"

rm -rf "$t/runs"; fakes "$t/f-other" "fixed breaks-other
"
printf '#!/bin/sh\ngrep -q fixed pool/PART.fixprobe@1/f.c && { echo fixed; exit 0; }\nexit 1\n' > "$r/tests/programs/fixprobe-red.sh"
MODEL_FAKE="$t/f-other" run fixprobe.red@1 && fail "a fix that broke another test was judged a pass"
grep -q '^result *= "fail"' "$t/runs"/*/run.toml || fail "a fix that broke another test was not recorded as a fail"

rm -rf "$t/runs"; fakes "$t/f-out" "fixed
" tests/programs/fixprobe-red.sh
MODEL_FAKE="$t/f-out" run fixprobe.red@1 && fail "a fix outside pool/ was judged"
grep -q '^result *= "refused"' "$t/runs"/*/run.toml && grep -q "only a file under pool/" "$t/said" || fail "a fix outside pool/ was not refused: $(tail -1 "$t/said")"
echo "a green test refused before any call; a test already red before the fix not held against it; the look answered from the repo and refused outside it; a read given as two numbers and a find kept to one folder; a good fix a pass with its 2 lines and 500 in, 100 out, 5000 from the cache, on its summary line; a fix that broke another test a fail; a fix outside pool/ refused unjudged; the model's copy untouched"
