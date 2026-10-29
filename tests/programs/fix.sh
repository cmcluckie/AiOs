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
#   a fix may hand in several files of one part at once (pool/PART.fixprobe@1/f.c
#     and g.S), their changed lines added up; files of two parts are refused
#   the copy the model worked from never changes: f.c still says "broken"
#   a test that needs an upstream clone beside the repo still finds it from the
#     judge's copy (fixprobe.clone), so nothing is skipped that runs in the repo
# Exit 0 green, 1 red, 2 where jig/test cannot run (no QEMU) or there is no jq.
set -u
cd "$(dirname "$0")/../.."
fail() { echo "$*"; exit 1; }
command -v qemu-system-x86_64 > /dev/null || { echo "no QEMU here, so jig/test does not run"; exit 2; }
command -v jq > /dev/null || { echo "no jq here"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
mkdir "$t/r"; tar --exclude=./.git --exclude=./out --exclude=./runs -cf - . | tar -xf - -C "$t/r"
r="$t/r"; mkdir -p "$r/pool/PART.fixprobe@1" "$r/pool/PART.fixprobe@2"; printf 'broken\n' > "$r/pool/PART.fixprobe@1/f.c"
printf 'one\n' > "$r/pool/PART.fixprobe@1/g.S"; printf 'other part\n' > "$r/pool/PART.fixprobe@2/f.c"
printf '#!/bin/sh\ngrep -qx fixed pool/PART.fixprobe@1/f.c && { echo fixed; exit 0; }\necho "f.c says $(head -1 pool/PART.fixprobe@1/f.c)"; exit 1\n' > "$r/tests/programs/fixprobe-red.sh"
printf '#!/bin/sh\ngrep -q breaks-other pool/PART.fixprobe@1/f.c && { echo "broken by the fix"; exit 1; }\necho fine; exit 0\n' > "$r/tests/programs/fixprobe-other.sh"
printf '#!/bin/sh\necho "red before any fix, and after"; exit 1\n' > "$r/tests/programs/fixprobe-always.sh"
# a test that fails when the bench's own settings reach the suite (the first 3090 run, decision 0089)
printf '#!/bin/sh\n[ -z "${MODEL_NAME:-}${FIX_PROMPT:-}${MODEL_NUM_CTX:-}" ] && { echo "clean"; exit 0; }\necho "the bench set MODEL_NAME=${MODEL_NAME:-} FIX_PROMPT=${FIX_PROMPT:-} MODEL_NUM_CTX=${MODEL_NUM_CTX:-}"; exit 1\n' > "$r/tests/programs/fixprobe-env.sh"
# a test that needs an upstream clone beside the repo, as the reader and origin tests do
mkdir -p "$t/musl"; printf '#!/bin/sh\n[ -n "${MUSL:-}" ] && [ -d "$MUSL" ] && { echo "found $MUSL"; exit 0; }\necho "no clone of musl"; exit 2\n' > "$r/tests/programs/fixprobe-clone.sh"
for n in red other always clone env; do
	printf 'code       = "TST.fixprobe.%s@1"\nstatus     = "draft"\nadmitted   = ""\nretired_by = ""\nbinds      = "SPEC.fixprobe.1@1"\nred_when   = "f.c is not fixed"\ntimeout_ms = 10000\nisolates   = "PART.fixprobe@1"\nrunner     = "jig/test cmd sh tests/programs/fixprobe-%s.sh"\nmutations  = 0\n' "$n" "$n" > "$r/tests/fixprobe.$n@1.toml"
done
printf 'code       = "SPEC.fixprobe.1@1"\nstatus     = "draft"\nadmitted   = ""\nretired_by = ""\ncomponent  = "PART.fixprobe@1"\nclause     = "f.c says fixed."\nauthor     = "KEY.chris"\n' > "$r/specs/fixprobe.1@1.toml"
reply() {	# reply FILE TOOL INPUT_JSON: a saved Converse reply that calls one tool
	printf '{"output":{"message":{"role":"assistant","content":[{"toolUse":{"toolUseId":"u-%s","name":"%s","input":%s}}]}},"stopReason":"tool_use","usage":{"inputTokens":100,"outputTokens":20,"cacheReadInputTokens":1000}}\n' "$(basename "$1" .json)" "$2" "$3" > "$1"
}
stop() {	# stop FILE: a saved reply that says something and hands in nothing
	printf '{"output":{"message":{"role":"assistant","content":[{"text":"I stop here."}]}},"stopReason":"end_turn","usage":{"inputTokens":100,"outputTokens":20}}\n' > "$1"
}
fakes() {	# fakes DIR CONTENT [PATH]: look at f.c, look outside, read lines 2 to 2 given as
	# two numbers, find a word in one folder, then hand in CONTENT
	mkdir -p "$1"
	reply "$1/1.json" look '{"why":"to see what is there","command":"read","args":["pool/PART.fixprobe@1/f.c"]}'
	reply "$1/2.json" look '{"why":"to see what is there","command":"read","args":["../../etc/passwd"]}'
	reply "$1/3.json" look '{"why":"to see what is there","command":"read","args":["tests/programs/fixprobe-red.sh","2","2"]}'
	reply "$1/4.json" look '{"why":"to see what is there","command":"find","args":["broken","pool/PART.fixprobe@1"]}'
	reply "$1/5.json" look '{"why":"to see what is there","command":"read","args":["tests/programs/fixprobe-red.sh"],"start_line":3,"end_line":3}'
	reply "$1/6.json" submit_fix "$(jq -cn --arg p "${3:-pool/PART.fixprobe@1/f.c}" --arg c "$2" '{path:$p,content:$c}')"
	stop "$1/7.json"	# if the hand-in is sent back, it stops there
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
grep -q '^input_tokens *= 600$' "$in/run.toml" && grep -q '^output_tokens *= 120$' "$in/run.toml" && grep -q '^cache_read_tokens *= 6000$' "$in/run.toml" || fail "the tokens of every call were not counted: $(grep '_tokens' "$in/run.toml" | tr '\n' ' ')"
grep -q '"text": *"pool/PART.fixprobe@1/f.c, lines 1 to 1 of 1\\n1\\tbroken' "$in/input.txt" || fail "the model's look at f.c was not answered with what f.c says"
grep -q 'outside the repo' "$in/input.txt" || fail "a look outside the repo was not refused"
grep -q '"text": *"tests/programs/fixprobe-red.sh, lines 2 to 2 of 3\\n2\\tgrep -qx fixed' "$in/input.txt" || fail "a read of lines 2 to 2, given as two numbers, did not give line 2 alone"
grep -q '"text": *"tests/programs/fixprobe-red.sh, lines 3 to 3 of 3\\n3\\techo' "$in/input.txt" || fail "a read given start_line and end_line, as Sonnet asked on try 2, did not give that line, headed with where it is in the file"
grep -q 'pool/PART.fixprobe@1/f.c:1:broken' "$in/input.txt" && ! grep -q 'tests/programs/fix.sh:[0-9]*:' "$in/input.txt" || fail "a find in one folder was not kept to that folder"
! grep -q 'FAIL  TST.fixprobe.env@1' "$t/said" || fail "the bench's own settings reached the suite it runs: $(grep 'fixprobe.env' "$t/said" | head -1)"
grep -q ', 0 skipped' "$t/said" || fail "a test that needs a clone beside the repo was skipped in the judge's copy: $(tail -1 "$t/said")"
grep -q '6000 read from the cache' "$t/said" || fail "the summary line leaves out the tokens read from the cache: $(tail -1 "$t/said")"
grep -qx broken "$r/pool/PART.fixprobe@1/f.c" || fail "the copy the model worked from was changed"
grep -q '^  call 1 of 60: read pool/PART.fixprobe@1/f.c' "$t/said" && grep -q '^  hand-in 1: pass' "$t/said" && grep -q '^    FAIL  TST.fixprobe.red@1' "$t/said" && grep -q '^    PASS  TST.fixprobe.red@1' "$t/said" || fail "the run did not say each call, each hand-in and each test of both suite runs as it went"
grep -q '"text": *"Call 1 of 60 done; 59 left."' "$in/input.txt" || fail "the model was not told after its first call how many of its 60 it has left"
# with a cap of 12, the second answer says ten are left and asks for the fix now
rm -rf "$t/runs"; fakes "$t/f-budget" "fixed
"
FIX_TURNS=12 MODEL_FAKE="$t/f-budget" run fixprobe.red@1 || fail "the run with 12 calls did not pass: $(tail -1 "$t/said")"
in=$(ls -d "$t/runs"/*/ | head -1)
grep -q '"text": *"Call 1 of 12 done; 11 left."' "$in/input.txt" || fail "with 11 left the model was not told so, or was hurried"
grep -q '"text": *"Call 2 of 12 done; 10 left.  Ten or fewer left: hand in your fix with submit_fix now.' "$in/input.txt" || fail "with ten left the model was not asked to hand in its fix now"
rm -rf "$t/runs"

rm -rf "$t/runs"; fakes "$t/f-other" "fixed breaks-other
"
printf '#!/bin/sh\ngrep -q fixed pool/PART.fixprobe@1/f.c && { echo fixed; exit 0; }\nexit 1\n' > "$r/tests/programs/fixprobe-red.sh"
MODEL_FAKE="$t/f-other" run fixprobe.red@1 && fail "a fix that broke another test was judged a pass"
grep -q '^result *= "fail"' "$t/runs"/*/run.toml || fail "a fix that broke another test was not recorded as a fail"
grep -q 'Hand-in 1 judged: fail' "$t/runs"/*/input.txt && grep -q 'What the tests that went red printed:' "$t/runs"/*/input.txt || fail "a fix that broke another test was not sent back with what the test printed"
# sent back, it hands in again, and the second is judged on its own: a pass
rm -rf "$t/runs"; mkdir -p "$t/f-again"
reply "$t/f-again/1.json" submit_fix '{"path":"pool/PART.fixprobe@1/f.c","content":"fixed breaks-other\n"}'
reply "$t/f-again/2.json" submit_fix '{"path":"pool/PART.fixprobe@1/f.c","content":"fixed\n"}'
MODEL_FAKE="$t/f-again" run fixprobe.red@1 || fail "a second hand-in, after the first broke a test, was not judged a pass: $(tail -1 "$t/said")"
grep -q '2 hand-in(s)' "$t/said" && grep -q '^lines *= 2$' "$t/runs"/*/run.toml || fail "the second hand-in was not counted, or its own lines were not the ones recorded: $(tail -1 "$t/said")"
grep -q 'fixprobe.other' "$t/runs"/*/input.txt || fail "the first hand-in was not answered with the test that went red"
# sent back, it stops with an empty reply; reminded once, it hands in again (loader.random try 2)
rm -rf "$t/runs"; mkdir -p "$t/f-nudge"
reply "$t/f-nudge/1.json" submit_fix '{"path":"pool/PART.fixprobe@1/f.c","content":"fixed breaks-other\n"}'
printf '{"output":{"message":{"role":"assistant","content":[]}},"stopReason":"end_turn","usage":{"inputTokens":100,"outputTokens":0}}\n' > "$t/f-nudge/2.json"
reply "$t/f-nudge/3.json" submit_fix '{"path":"pool/PART.fixprobe@1/f.c","content":"fixed\n"}'
MODEL_FAKE="$t/f-nudge" run fixprobe.red@1 || fail "a model that stopped with an empty reply was not reminded and given its second hand-in: $(tail -1 "$t/said")"
grep -q 'You stopped without handing in a corrected fix' "$t/runs"/*/input.txt && grep -q '(the reply was empty)' "$t/runs"/*/input.txt || fail "the reminder, or the note that the reply was empty, is not in the run's record"

rm -rf "$t/runs"; fakes "$t/f-out" "fixed
" tests/programs/fixprobe-red.sh
MODEL_FAKE="$t/f-out" run fixprobe.red@1 && fail "a fix outside pool/ was judged"
grep -q '^result *= "refused"' "$t/runs"/*/run.toml && grep -q "only a file under pool/" "$t/said" || fail "a fix outside pool/ was not refused: $(tail -1 "$t/said")"
# a fix that needs two files of the same part hands both in at once
rm -rf "$t/runs"; mkdir -p "$t/f-two"
reply "$t/f-two/1.json" submit_fix '{"files":[{"path":"pool/PART.fixprobe@1/f.c","content":"fixed\n"},{"path":"pool/PART.fixprobe@1/g.S","content":"two\n"}]}'
MODEL_FAKE="$t/f-two" run fixprobe.red@1 || fail "a fix in two files of one part was not judged a pass: $(tail -1 "$t/said")"
grep -q '^lines *= 4$' "$t/runs"/*/run.toml || fail "the changed lines of both files were not counted: $(grep '^lines' "$t/runs"/*/run.toml)"
grep -q 'g.S' "$t/runs"/*/output.txt || fail "the second file's change is not in the run's output"
# a reviewer's note goes to the model with the test; a note that is not there stops it before any call
rm -rf "$t/runs"; fakes "$t/f-note" "fixed
"
printf 'check the flag before you use it\n' > "$t/note"
FIX_NOTE="$t/note" MODEL_FAKE="$t/f-note" run fixprobe.red@1 || fail "a run with a reviewer's note did not pass: $(tail -1 "$t/said")"
grep -q 'A person read an earlier fix for this test and wrote:' "$t/runs"/*/input.txt && grep -q 'check the flag before you use it' "$t/runs"/*/input.txt || fail "the reviewer's note did not reach the model"
rm -rf "$t/runs"; mkdir -p "$t/f-nonote"
FIX_NOTE="$t/no-such-note" MODEL_FAKE="$t/f-nonote" run fixprobe.red@1 && fail "a run with a missing note went ahead"
[ ! -e "$t/f-nonote/.n" ] || fail "a run with a missing note made a call"
# the model cannot be reached after the first call: the calls already paid for are recorded
rm -rf "$t/runs"; mkdir -p "$t/f-gone"
reply "$t/f-gone/1.json" look '{"why":"to see what is there","command":"read","args":["pool/PART.fixprobe@1/f.c"]}'
MODEL_FAKE="$t/f-gone" run fixprobe.red@1; rc=$?
[ $rc -eq 1 ] && grep -q '^result *= "refused"' "$t/runs"/*/run.toml 2> /dev/null && grep -q 'could not be reached on call 2' "$t/said" || fail "a model lost after one call was not recorded as refused with the calls it cost ($rc): $(tail -1 "$t/said")"
grep -q '^input_tokens *= 100$' "$t/runs"/*/run.toml || fail "the call made before the model was lost was not counted"
# two files in two different parts is refused, nothing judged
rm -rf "$t/runs"; mkdir -p "$t/f-split"
reply "$t/f-split/1.json" submit_fix '{"files":[{"path":"pool/PART.fixprobe@1/f.c","content":"fixed\n"},{"path":"pool/PART.fixprobe@2/f.c","content":"x\n"}]}'
stop "$t/f-split/2.json"
MODEL_FAKE="$t/f-split" run fixprobe.red@1 && fail "a fix across two parts was judged"
grep -q '^result *= "refused"' "$t/runs"/*/run.toml && grep -q "one part" "$t/said" || fail "a fix across two parts was not refused: $(tail -1 "$t/said")"
# a reply cut off at the token limit is said to be, and the reminder says why (decision 0086)
rm -rf "$t/runs"; mkdir -p "$t/f-cut"
printf '{"output":{"message":{"role":"assistant","content":[]}},"stopReason":"max_tokens","usage":{"inputTokens":100,"outputTokens":16000}}\n' > "$t/f-cut/1.json"
reply "$t/f-cut/2.json" submit_fix '{"path":"pool/PART.fixprobe@1/f.c","content":"fixed\n"}'
MODEL_FAKE="$t/f-cut" run fixprobe.red@1 || fail "a model whose first reply was cut off was not given its next call: $(tail -1 "$t/said")"
grep -q 'the reply was cut off at 16000 tokens' "$t/said" && grep -q 'Your reply was cut off at 16000 tokens' "$t/runs"/*/input.txt || fail "a reply cut off at the limit was not said to be, or the model was not told why it was reminded"
# a conversation that outgrew the model's context ends the run, said as that, not as a cut-off reply (decision 0090)
rm -rf "$t/runs"; mkdir -p "$t/f-full"
reply "$t/f-full/1.json" look '{"why":"to see what is there","command":"read","args":["pool/PART.fixprobe@1/f.c"]}'
printf '{"output":{"message":{"role":"assistant","content":[]}},"stopReason":"model_context_window_exceeded","usage":{"inputTokens":49000,"outputTokens":150}}\n' > "$t/f-full/2.json"
reply "$t/f-full/3.json" submit_fix '{"path":"pool/PART.fixprobe@1/f.c","content":"fixed\n"}'
MODEL_FAKE="$t/f-full" run fixprobe.red@1 && fail "a run whose conversation outgrew the context went on to a pass"
grep -q "the conversation outgrew the model's context" "$t/said" && ! grep -q 'cut off at' "$t/said" && ! grep -q 'reminded once' "$t/said" || fail "a full context was not said as that, or was taken for a cut-off reply, or reminded: $(tail -3 "$t/said" | tr '\n' ' ')"
grep -q '^result *= "refused"' "$t/runs"/*/run.toml || fail "a run ended by a full context was not recorded as refused"
# ReAct: a look must say why, or it is refused and not run; the why is said as it goes.
# A look's answer is cut at FIX_LOOK_CHARS (8000), with a marker that says so (decision 0092)
rm -rf "$t/runs"; mkdir -p "$t/f-why"
awk 'BEGIN { for (i = 1; i <= 400; i++) printf "line %04d of a long file, padded out to sixty characters...\n", i }' > "$r/pool/PART.fixprobe@1/long.c"
reply "$t/f-why/1.json" look '{"command":"read","args":["pool/PART.fixprobe@1/f.c"]}'
reply "$t/f-why/2.json" look '{"why":"to find the broken word","command":"read","args":["pool/PART.fixprobe@1/long.c"]}'
reply "$t/f-why/3.json" submit_fix '{"path":"pool/PART.fixprobe@1/f.c","content":"fixed\n"}'
MODEL_FAKE="$t/f-why" run fixprobe.red@1 || fail "a run with a look that gave no why did not go on to its fix: $(tail -1 "$t/said")"
in=$(ls -d "$t/runs"/*/ | head -1)
grep -q '"text": *"refused: say why you are looking' "$in/input.txt" && ! grep -q '"text": *"pool/PART.fixprobe@1/f.c, lines 1 to 1 of 1' "$in/input.txt" || fail "a look with no why was answered, or not refused with a reason"
grep -q 'why: to find the broken word' "$t/said" || fail "the why of a look was not said as it went"
grep -q 'cut at 8000 characters' "$in/input.txt" && ! grep -q 'line 0400 of a long file' "$in/input.txt" || fail "a long read was not cut at 8000 characters with a marker that says so"
rm -f "$r/pool/PART.fixprobe@1/long.c"
# FIX_AT: the repo as it was at a commit, not as it is; nothing in runs/ or added later is there to read
rm -rf "$t/runs"
printf 'broken\n' > "$r/pool/PART.fixprobe@1/f.c"; mkdir -p "$r/runs/old"; printf 'the answer was fixed\n' > "$r/runs/old/output.txt"
printf '#!/bin/sh\ngrep -qx fixed pool/PART.fixprobe@1/f.c && { echo fixed; exit 0; }\necho "f.c says $(head -1 pool/PART.fixprobe@1/f.c)"; exit 1\n' > "$r/tests/programs/fixprobe-red.sh"
# a copy that can see the repo's history checks the repo's later commits against its own
# older decisions, and refuses them (the first 3090 run, decision 0088)
printf '#!/bin/sh\ngit rev-parse --show-toplevel > /dev/null 2>&1 && { echo "the copy sees the history of $(git rev-parse --show-toplevel)"; exit 1; }\necho "no history here"; exit 0\n' > "$r/tests/programs/fixprobe-git.sh"
printf 'code       = "TST.fixprobe.git@1"\nstatus     = "draft"\nadmitted   = ""\nretired_by = ""\nbinds      = "SPEC.fixprobe.1@1"\nred_when   = "the copy sees a history"\ntimeout_ms = 10000\nisolates   = "PART.fixprobe@1"\nrunner     = "jig/test cmd sh tests/programs/fixprobe-git.sh"\nmutations  = 0\n' > "$r/tests/fixprobe.git@1.toml"
git -C "$r" init -q && git -C "$r" add -A && git -C "$r" -c user.name=p -c user.email=p@p commit -qm before || fail "could not make the probe's history"
before=$(git -C "$r" rev-parse --short HEAD)
printf 'fixed\n' > "$r/pool/PART.fixprobe@1/f.c"; printf 'the answer is fixed\n' > "$r/findings/fixprobe-answer.md"
git -C "$r" add -A && git -C "$r" -c user.name=p -c user.email=p@p commit -qm after || fail "could not make the probe's history"
mkdir -p "$t/f-at"
reply "$t/f-at/1.json" look '{"why":"to see what is there","command":"read","args":["pool/PART.fixprobe@1/f.c"]}'
reply "$t/f-at/2.json" look '{"why":"to see what is there","command":"read","args":["runs/old/output.txt"]}'
reply "$t/f-at/3.json" look '{"why":"to see what is there","command":"read","args":["findings/fixprobe-answer.md"]}'
reply "$t/f-at/4.json" submit_fix '{"path":"pool/PART.fixprobe@1/f.c","content":"fixed\n"}'
FIX_AT=$before FIX_OUT="$r/out/fix" MODEL_FAKE="$t/f-at" run fixprobe.red@1 || fail "a fix made again from the commit before it was not judged a pass: $(tail -1 "$t/said")"
grep -q "as it was at $before" "$t/said" && grep -q "at $before:" "$t/said" || fail "the run did not say it worked from $before: $(tail -1 "$t/said")"
grep -q "^at *= \"$before\"$" "$t/runs"/*/run.toml || fail "the run did not record the commit it worked from: $(grep '^at' "$t/runs"/*/run.toml)"
grep -q '"text": *"pool/PART.fixprobe@1/f.c, lines 1 to 1 of 1\\n1\\tbroken' "$t/runs"/*/input.txt || fail "the model did not see f.c as it was at $before"
grep -q '"text": *"no file runs/old/output.txt"' "$t/runs"/*/input.txt && grep -q '"text": *"no file findings/fixprobe-answer.md"' "$t/runs"/*/input.txt || fail "the model could read a run record, or a file added after $before"
! grep -q 'FAIL  TST.fixprobe.git@1' "$t/said" || fail "a copy at FIX_AT, inside the repo's folder, could see the repo's history: $(grep 'fixprobe.git' "$t/said" | head -1)"
grep -qx fixed "$r/pool/PART.fixprobe@1/f.c" || fail "the repo itself was changed"
echo "a green test refused before any call; a fix in two files of one part a pass with both files' 4 lines, one across two parts refused; a test already red before the fix not held against it; the look answered from the repo and refused outside it; a read given as two numbers, or as start_line and end_line, headed with where it is; a find kept to one folder; a good fix a pass with its 2 lines and 600 in, 120 out, 6000 from the cache, on its summary line; nothing skipped in the judge's copy that runs beside the clones; a fix that broke another test a fail; a fix outside pool/ refused unjudged; a fix that broke a test sent back with what it printed, and a second hand-in judged a pass; the model's copy untouched; after every call the model told how many it has left, and with ten left asked for its fix; each call and hand-in said as it went; a model lost after its first call recorded with what it cost; a reviewer's note handed over with the test, a missing one refused before any call; a model that stops without a fix reminded once; a reply cut off at the limit said to be, and the model told why; a conversation that outgrew the context ended and said as that; a look with no why refused, a why said as it went, a long read cut at 8000 characters with a marker; a fix made again from the commit before it, the model shown that commit and no run record or later file, nor the repo's history, and the commit recorded"
