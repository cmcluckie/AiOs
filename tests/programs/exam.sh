#!/bin/sh
# Probe for TST.exam.*@1: jig/runs and jig/scorecard on small fixtures in
# tests/fixtures/exam, nothing from the real runs/.  One mode per test:
#   count      code lines are counted the same way for every file
#   record     a run is recorded with its cost, or refused with a reason
#   budget     the monthly cap refuses a call that could pass it
#   scorecard  the table for two models matches tests/expected/scorecard.table@1.out
#   judge      a lift stands in for the hand lift and the item's tests decide
# Exit 0 is green, 1 is red, 2 means this host cannot run it (no QEMU).
set -u
cd "$(dirname "$0")/../.."
mode=${1:-}
fx=tests/fixtures/exam
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
export RUNS="$tmp/runs" PRICES="$fx/prices.toml" BUDGET="$fx/budget.toml"
fail() { echo "$*"; exit 1; }
runs() { sh jig/runs "$@"; }
# refuse WHAT FRAGMENT COMMAND...: exit status 1, FRAGMENT on stderr, nothing recorded
refuse() {
	what=$1; frag=$2; shift 2
	"$@" > "$tmp/out" 2> "$tmp/err"; rc=$?
	[ $rc -eq 1 ] || fail "$what: exit status $rc, not a refusal"
	grep -q -- "$frag" "$tmp/err" || fail "$what: refused, but the reason says '$(head -1 "$tmp/err")', not '$frag'"
}
# rec NOW ITEM MODEL VERSION TRY IN OUT RESULT LINES
rec() { RUNS_NOW=$1 runs record "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9" "$fx/sample.in" "$fx/sample.out"; }

case "$mode" in
count)
	[ "$(runs count $fx/hand-serial.c)" = 10 ] || fail "hand-serial.c has 10 code lines, count says $(runs count $fx/hand-serial.c)"
	[ "$(runs count $fx/hand-timer.c)" = 6 ] || fail "hand-timer.c has 6 code lines, count says $(runs count $fx/hand-timer.c)"
	[ "$(runs count $fx/hand-screen.c)" = 4 ] || fail "hand-screen.c has 4 code lines, count says $(runs count $fx/hand-screen.c)"
	[ "$(runs count $fx/hand-serial.c $fx/hand-timer.c)" = 16 ] || fail "two files together should be 16, count says $(runs count $fx/hand-serial.c $fx/hand-timer.c)"
	refuse "a file that is not there" "is not a file" sh jig/runs count $fx/nosuch.c
	echo "10, 6 and 4 code lines in the three fixtures, 16 for two together: blanks, one-line and two-line block comments, // comments and a comment in the middle of a line are not counted; a code line with a trailing comment is" ;;
record)
	id=$(rec 2026-10-17T10:00:00Z serial test:big v1 1 1000 2000 fail 9) || fail "a good run was refused"
	[ "$id" = "20261017T100000Z-serial-1-test-big" ] || fail "the id is '$id'"
	r="$RUNS/$id"
	grep -q '^cost_usd      = 0.033000$' "$r/run.toml" || fail "1000 in and 2000 out at 3 and 15 dollars a million should cost 0.033000: $(grep cost_usd "$r/run.toml")"
	grep -qx 'sitting       = "20261017T100000Z"' "$r/run.toml" && grep -qx 'prompt        = "-"' "$r/run.toml" || fail "a run recorded with no sitting or prompt given should be its own sitting, named by its time, with prompt '-': $(grep -E 'sitting|prompt' "$r/run.toml" | tr '\n' ' ')"
	for kv in 'item          = "serial"' 'model         = "test:big"' 'version       = "v1"' 'try           = 1' 'input_tokens  = 1000' 'output_tokens = 2000' 'result        = "fail"' 'lines         = 9'; do
		grep -q "^$kv\$" "$r/run.toml" || fail "run.toml lacks: $kv"
	done
	cmp -s "$r/input.txt" $fx/sample.in && cmp -s "$r/output.txt" $fx/sample.out || fail "the input and output were not kept as they were"
	refuse "a model with no price" "no price for test:free" env RUNS_NOW=2026-10-17T10:01:00Z sh jig/runs record serial test:free v1 1 1 1 pass 1 $fx/sample.in $fx/sample.out
	refuse "the same run twice" "already recorded" env RUNS_NOW=2026-10-17T10:00:00Z sh jig/runs record serial test:big v1 1 1000 2000 fail 9 $fx/sample.in $fx/sample.out
	refuse "a result that is not pass, fail or refused" "RESULT is" rec 2026-10-17T10:02:00Z serial test:big v1 1 1 1 maybe 1
	refuse "try 0" "TRY starts at 1" rec 2026-10-17T10:03:00Z serial test:big v1 0 1 1 pass 1
	refuse "tokens that are not a number" "IN_TOKENS must be a whole number" rec 2026-10-17T10:04:00Z serial test:big v1 1 lots 1 pass 1
	refuse "an input file that is not there" "is not a file" env RUNS_NOW=2026-10-17T10:05:00Z sh jig/runs record serial test:big v1 1 1 1 pass 1 $fx/nosuch.in $fx/sample.out
	[ "$(ls "$RUNS" | wc -l)" -eq 1 ] || fail "$(ls "$RUNS" | wc -l) runs on disk after six refusals; the refused ones left something behind"
	echo "one run recorded with cost 0.033000 (1000 in, 2000 out), its input and output kept as they were; a model with no price, a repeat, a result that is not pass, fail or refused, try 0, tokens that are words and a missing input were each refused and left nothing behind" ;;
budget)
	rec 2026-10-02T09:00:00Z serial test:big v1 1 0 60000 pass 5 > /dev/null || fail "the October run was refused"
	rec 2026-09-30T09:00:00Z serial test:big v1 1 0 20000 pass 5 > /dev/null || fail "the September run was refused"
	[ "$(runs spent 2026-10)" = 0.900000 ] || fail "October spent should be 0.900000, spent says $(runs spent 2026-10)"
	[ "$(runs spent 2026-09)" = 0.300000 ] || fail "September spent should be 0.300000, spent says $(runs spent 2026-09)"
	[ "$(runs spent 2026-08)" = 0.000000 ] || fail "August spent should be 0.000000, spent says $(runs spent 2026-08)"
	export RUNS_NOW=2026-10-17T12:00:00Z
	runs budget 0.05 > "$tmp/out" 2>&1 || fail "0.05 on top of 0.90 under a cap of 1.00 was refused: $(cat "$tmp/out")"
	runs budget 0.10 > "$tmp/out" 2>&1 || fail "0.10 on top of 0.90 is exactly the cap and was refused: $(cat "$tmp/out")"
	refuse "a call that could pass the cap" "would pass the cap of 1.00" sh jig/runs budget 0.15
	refuse "a ceiling that is not dollars" "CEILING must be dollars" sh jig/runs budget lots
	echo "October's 0.900000 is counted and September's 0.300000 is not; a call that could cost 0.05 or 0.10 is let through and 0.15 is refused against the 1.00 cap" ;;
scorecard)
	R() { rec "$@" > /dev/null || fail "run '$*' was refused"; }
	export RUNS_SITTING=20261017T100000Z RUNS_PROMPT=aaaa1111
	R 2026-10-17T10:00:00Z serial test:big v1 1 1000 2000 fail 9
	R 2026-10-17T10:01:00Z serial test:big v1 2 1000 1500 pass 8
	export RUNS_SITTING=20261017T103000Z RUNS_PROMPT=bbbb2222
	R 2026-10-17T10:30:00Z serial test:big v1 1 500 200 pass 7
	unset RUNS_SITTING
	R 2026-10-17T10:40:00Z timer test:big v1 1 2000 1000 pass 5
	export RUNS_SITTING=20261017T104500Z
	R 2026-10-17T10:45:00Z timer test:big v1 1 1000 1000 fail 0
	unset RUNS_PROMPT RUNS_SITTING
	R 2026-10-17T11:00:00Z serial test:local v2 1 5000 500 pass 12
	R 2026-10-17T11:05:00Z serial test:local v2 1 5000 500 fail 0
	for old in "$RUNS"/*-serial-1-*; do
		case "$old" in *110000Z* | *110500Z*) sed -i '/^sitting /d;/^prompt /d' "$old/run.toml" ;; esac
	done
	export RUNS_SITTING=20261017T110100Z
	R 2026-10-17T11:01:00Z timer test:local v2 1 100 100 fail 0
	R 2026-10-17T11:02:00Z timer test:local v2 2 100 100 fail 0
	R 2026-10-17T11:03:00Z timer test:local v2 3 100 100 fail 0
	unset RUNS_SITTING
	EXAM="$fx/items.toml" EXAM_ROOT="$fx" sh jig/scorecard > "$tmp/got" 2>&1 || fail "the scorecard failed: $(head -2 "$tmp/got")"
	cmp -s "$tmp/got" tests/expected/scorecard.table@1.out || fail "the scorecard differs from tests/expected/scorecard.table@1.out: $(diff "$tmp/got" tests/expected/scorecard.table@1.out | head -2 | tr '\n' ' ')"
	rm -rf "$RUNS"; mkdir "$RUNS"
	EXAM="$fx/items.toml" EXAM_ROOT="$fx" sh jig/scorecard > "$tmp/got" 2>&1 || fail "the scorecard failed on no runs"
	[ "$(cat "$tmp/got")" = "no runs recorded in $RUNS" ] || fail "with no runs it said '$(head -1 "$tmp/got")'"
	echo "two models, eleven runs in seven sittings: the table matches the one written by hand (a line per sitting with its passes, lines beside the hand lift's, tries, cost including the failed tries and prompt fingerprint, an item never sat, and the cost per passing sitting), and with no runs it says so" ;;
judge)
	command -v qemu-system-x86_64 > /dev/null || { echo "no qemu-system-x86_64"; exit 2; }
	hand=lifted/PART.serial.8250@1
	before=$(cksum < $hand/serial8250.c)
	sh jig/judge serial $hand > "$tmp/out" 2>&1; rc=$?
	[ $rc -eq 0 ] && [ "$(cat "$tmp/out")" = "pass 57 lines" ] || fail "the hand lift itself should pass with 57 lines: exit $rc, '$(head -1 "$tmp/out")'"
	# a lift that never waits for the chip to be ready
	mkdir "$tmp/nowait"; cp $hand/serial8250.c "$tmp/nowait/"
	sed -i 's/while ((inb(base + UART_LSR) \& UART_LSR_THRE) == 0)/while (0)/' "$tmp/nowait/serial8250.c"
	cmp -s $hand/serial8250.c "$tmp/nowait/serial8250.c" && fail "the fixture edit changed nothing"
	sh jig/judge serial "$tmp/nowait" > "$tmp/out" 2>&1; rc=$?
	[ $rc -eq 1 ] && grep -q '^fail .* lines: serial.waits-ready@1' "$tmp/out" || fail "a lift that never waits for the chip: exit $rc, '$(head -1 "$tmp/out")'"
	# a lift that sends every newline twice
	mkdir "$tmp/twice"; cp $hand/serial8250.c "$tmp/twice/"
	sed -i 's/^\toutb(c, base + UART_TX);/\tif (c == 10) outb(13, base + UART_TX);\n\toutb(c, base + UART_TX);/' "$tmp/twice/serial8250.c"
	cmp -s $hand/serial8250.c "$tmp/twice/serial8250.c" && fail "the second fixture edit changed nothing"
	sh jig/judge serial "$tmp/twice" > "$tmp/out" 2>&1; rc=$?
	[ $rc -eq 1 ] && grep -q '^fail 58 lines: serial.in-order@1' "$tmp/out" || fail "a lift that adds a byte: exit $rc, '$(head -1 "$tmp/out")'"
	# a lift that is not C
	mkdir "$tmp/junk"; cp $hand/serial8250.c "$tmp/junk/"; echo 'this is not C' >> "$tmp/junk/serial8250.c"
	sh jig/judge serial "$tmp/junk" > "$tmp/out" 2>&1; rc=$?
	[ $rc -eq 1 ] && grep -q '^fail ' "$tmp/out" || fail "a lift that is not C: exit $rc, '$(head -1 "$tmp/out")'"
	mkdir "$tmp/empty"
	refuse "a folder without the lift's file" "has no serial8250.c" sh jig/judge serial "$tmp/empty"
	sh jig/judge nosuchitem "$tmp/empty" > "$tmp/out" 2>&1; [ $? -eq 2 ] || fail "an item that is not in the exam should be 'cannot judge' (exit 2)"
	[ "$(cksum < $hand/serial8250.c)" = "$before" ] || fail "judging changed the hand lift in the repo"
	echo "the hand lift passes at 57 lines; a lift that never waits for the chip fails on serial.waits-ready@1, one that adds a byte fails on serial.in-order@1, one that is not C fails; a missing file is refused, an unknown item is 'cannot judge', and the repo's own hand lift is never touched" ;;
sit)
	command -v jq > /dev/null || { echo "jq is not installed"; exit 2; }
	sfx=tests/fixtures/sit
	export EXAM="$sfx/items.toml" SIT_OUT="$tmp/sit" PRICES="$fx/../model/prices.toml" MODEL_NAME=test:model
	export READER="sh $sfx/reader.sh" JUDGE="sh $sfx/judge.sh"
	cp -r $sfx/fake "$tmp/fake"; export MODEL_FAKE="$tmp/fake"
	sh jig/sit part > "$tmp/out" 2>&1; rc=$?
	[ $rc -eq 0 ] || fail "a model that passes on its second try gave exit $rc: $(tr '\n' ' ' < "$tmp/out")"
	grep -q '^try 1: fail (fail 7 lines: serial.waits-ready@1)  300 in, 50 out$' "$tmp/out" || fail "try 1 should fail on serial.waits-ready@1 with 300 in and 50 out: $(head -1 "$tmp/out")"
	grep -q '^try 2: pass (pass 5 lines)  300 in (1000 read from cache, 500 written to it), 40 out$' "$tmp/out" || fail "try 2 should pass at 5 lines with 300 in, 1000 read from the cache, 500 written to it and 40 out: $(sed -n 2p "$tmp/out")"
	grep -qx 'cost_usd      = 0.002450' "$RUNS"/*-part-2-*/run.toml || fail "try 2 should cost 0.002450 with its cached tokens at their own prices: $(grep -h cost_usd "$RUNS"/*-part-2-*/run.toml)"
	[ "$(jq '[.. | objects | select(has("cachePoint"))] | length' "$tmp/sit/part-try2/req.json")" = 3 ] && [ "$(jq '.messages[:-1] | [.. | objects | select(has("cachePoint"))] | length' "$tmp/sit/part-try2/req.json")" = 0 ] || fail "the request should mark the prompt, the tools and the last message to be cached, and nothing else"
	[ "$(jq '[.. | objects | select(has("cachePoint"))] | length' "$tmp/sit/part-try2/messages.json")" = 0 ] || fail "the markers were saved into the conversation, where they would pile up"
	[ "$(ls "$RUNS" | wc -l)" -eq 2 ] || fail "$(ls "$RUNS" | wc -l) runs recorded, not 2"
	grep -qx 'cost_usd      = 0.001100' "$RUNS"/*-part-1-*/run.toml || fail "try 1 should have cost 0.001100: $(grep -h cost_usd "$RUNS"/*-part-1-*/run.toml)"
	s1=$(sed -n 's/^sitting *= *"\(.*\)"$/\1/p' "$RUNS"/*-part-1-*/run.toml); s2=$(sed -n 's/^sitting *= *"\(.*\)"$/\1/p' "$RUNS"/*-part-2-*/run.toml)
	[ -n "$s1" ] && [ "$s1" = "$s2" ] || fail "the two tries of one start should share a sitting: '$s1' and '$s2'"
	grep -Eq '^prompt +\= "[0-9a-f]{8}"$' "$RUNS"/*-part-1-*/run.toml && grep -Eq '^prompt +\= "[0-9a-f]{8}"$' "$RUNS"/*-part-2-*/run.toml || fail "each try should carry the first 8 characters of the prompt's git hash"
	grep -qx 'result        = "fail"' "$RUNS"/*-part-1-*/run.toml && grep -qx 'result        = "pass"' "$RUNS"/*-part-2-*/run.toml && grep -qx 'lines         = 5' "$RUNS"/*-part-2-*/run.toml || fail "the two runs are not recorded as fail then pass at 5 lines"
	grep -q 'stub line from the reader' "$tmp/sit/part-try1/messages.json" || fail "what the reader said did not reach the model"
	grep -q "the reader has no command 'rm'" "$tmp/sit/part-try1/messages.json" || fail "a command the reader does not have was not turned down"
	grep -q 'Your last try failed: fail 7 lines: serial.waits-ready@1' "$tmp/sit/part-try2/first.txt" && grep -q 'BAD lift' "$tmp/sit/part-try2/first.txt" || fail "try 2 was not shown the failing test and its own file"
	[ "$(cat "$tmp/sit/part-try2/lift/part.c")" = "GOOD new lift" ] && [ ! -e "$tmp/sit/part-try2/lift/other.c" ] || fail "the file written is not exactly the one asked for, without fences"
	grep -q 'The fixed header part.h' "$tmp/sit/part-try1/first.txt" || fail "the model was not shown the header"
	# a model that writes no file is recorded as refused, with its cost
	rm -rf "$tmp/runs" "$tmp/sit"; cp -r $sfx/fake-refused "$tmp/fake2"
	MODEL_FAKE="$tmp/fake2" SIT_TRIES=1 sh jig/sit part > "$tmp/out" 2>&1; rc=$?
	[ $rc -eq 1 ] && grep -q '^try 1: refused (no usable file: part.c)  50 in, 10 out$' "$tmp/out" || fail "a reply with no file: exit $rc, '$(head -1 "$tmp/out")'"
	grep -qx 'result        = "refused"' "$RUNS"/*/run.toml || fail "the empty reply was not recorded as refused"
	# nothing is spent when the judge cannot pass the hand lift
	rm -rf "$tmp/runs" "$tmp/sit"; cp -r $sfx/fake "$tmp/fake3"
	printf 'exit 1\n' > "$tmp/badjudge.sh"
	JUDGE="sh $tmp/badjudge.sh" MODEL_FAKE="$tmp/fake3" sh jig/sit part > "$tmp/out" 2>&1; rc=$?
	[ $rc -eq 2 ] && [ ! -e "$tmp/fake3/.n" ] && [ ! -d "$RUNS" ] || fail "a judge that fails the hand lift: exit $rc, a call was made or a run recorded"
	echo "a model that fails then passes is recorded as two runs, fail then pass at 5 lines, costing 0.001100 for the first; the reader's answers and a refused command reach it, the second try is shown the failing test and its own file, only the file asked for is kept and fences are dropped; a reply with no file is recorded as refused with its cost; a judge that fails the hand lift stops it before any call" ;;
cache)
	export RUNS_NOW=2026-10-17T10:00:00Z
	RUNS_SITTING=sit-9 RUNS_PROMPT=pr-9 RUNS_CACHE_READ=10000 RUNS_CACHE_WRITE=2000 runs record serial test:cache v1 1 1000 500 pass 8 "$fx/sample.in" "$fx/sample.out" > /dev/null || fail "a run with cached tokens was refused"
	r="$RUNS/20261017T100000Z-serial-1-test-cache"
	grep -qx 'cost_usd      = 0.021000' "$r/run.toml" || fail "1000 in, 500 out, 10000 read and 2000 written at 3, 15, 0.30 and 3.75 should cost 0.021000: $(grep cost_usd "$r/run.toml")"
	grep -qx 'cache_read_tokens  = 10000' "$r/run.toml" && grep -qx 'cache_write_tokens = 2000' "$r/run.toml" || fail "the cached tokens were not written into run.toml"
	grep -qx 'sitting       = "sit-9"' "$r/run.toml" && grep -qx 'prompt        = "pr-9"' "$r/run.toml" || fail "the sitting and prompt given were not written into run.toml: $(grep -E 'sitting|prompt' "$r/run.toml" | tr '\n' ' ')"
	refuse "a sitting name with a space in it" "letters, digits" env RUNS_NOW=2026-10-17T10:04:00Z "RUNS_SITTING=a b" sh jig/runs record serial test:cache v1 1 1 1 pass 1 $fx/sample.in $fx/sample.out
	refuse "cached tokens for a model with no cache price" "no cache price for test:big" env RUNS_NOW=2026-10-17T10:01:00Z RUNS_CACHE_READ=5 sh jig/runs record serial test:big v1 1 1 1 pass 1 $fx/sample.in $fx/sample.out
	refuse "cached tokens that are words" "RUNS_CACHE_READ must be a whole number" env RUNS_NOW=2026-10-17T10:02:00Z RUNS_CACHE_READ=lots sh jig/runs record serial test:cache v1 1 1 1 pass 1 $fx/sample.in $fx/sample.out
	RUNS_NOW=2026-10-17T10:03:00Z runs record serial test:big v1 1 1000 2000 fail 9 "$fx/sample.in" "$fx/sample.out" > /dev/null || fail "a run with no cached tokens was refused for a model with no cache price"
	[ "$(ls "$RUNS" | wc -l)" -eq 2 ] || fail "$(ls "$RUNS" | wc -l) runs on disk, not 2: a refused run left something behind"
	echo "cached tokens are costed at their own prices (1000 in, 500 out, 10000 read, 2000 written: 0.021000) and kept in run.toml; cached tokens for a model with no cache price, or that are words, are refused and leave nothing behind; a run with none still records for a model with no cache price" ;;
ping)
	mfx=tests/fixtures/model
	export PRICES="$mfx/prices.toml" MODEL_NAME=test:model MODEL_FAKE="$mfx/ping.json"
	sh jig/model ping > "$tmp/out" 2>&1 || fail "a saved reply was refused: $(head -1 "$tmp/out")"
	grep -qx 'reply: ready' "$tmp/out" && grep -qx 'tokens: 14 in, 6 out' "$tmp/out" && grep -qx 'cost: 0.000088' "$tmp/out" || fail "the reply, tokens or cost are wrong: $(tr '\n' ' ' < "$tmp/out")"
	[ "$(ls "$RUNS" | wc -l)" -eq 1 ] && grep -q '^item *= "ping"$' "$RUNS"/*/run.toml && grep -q '^cost_usd *= 0.000088$' "$RUNS"/*/run.toml || fail "the call was not recorded once, as ping, with its cost"
	refuse "a model with no price" "no price for test:none" env MODEL_NAME=test:none sh jig/model ping
	[ "$(ls "$RUNS" | wc -l)" -eq 1 ] || fail "a refused call left a run behind"
	refuse "a call that could pass the cap" "would pass the cap" env MODEL_MAX_COST=5.00 sh jig/model ping
	env -u MODEL_FAKE AIOS_AWS="$tmp/nokey" sh jig/model ping > "$tmp/out" 2>&1; rc=$?
	[ $rc -eq 2 ] && grep -q "no key file" "$tmp/out" || fail "no key file gave exit $rc, not 'cannot run here'"
	printf 'AWS_ACCESS_KEY_ID=x\n' > "$tmp/open"; chmod 644 "$tmp/open"
	refuse "a key file others can read" "can be read by others" env -u MODEL_FAKE AIOS_AWS="$tmp/open" sh jig/model ping
	echo "a saved reply gives its word, 14 in and 6 out and a cost of 0.000088 and is recorded once as ping; a model with no price, a call that could pass the cap, a missing key file and a key file others can read each stop it before any call" ;;
ollama)
	mfx=tests/fixtures/model
	export PRICES="$mfx/prices.toml" MODEL_NAME=ollama:qwen-test OLLAMA_FAKE="$mfx/ollama/reply.json" OLLAMA_SENT="$tmp/sent.json"
	sh jig/model converse "$mfx/ollama/req.json" "$tmp/reply.json" > "$tmp/out" 2>&1 || fail "a local model's saved reply was refused: $(head -1 "$tmp/out")"
	s="$tmp/sent.json"; r="$tmp/reply.json"
	[ -s "$s" ] || fail "nothing was sent to Ollama"
	jq -e '.model == "qwen-test" and .stream == false and .options.num_ctx == 32768 and .options.num_predict == 2000' "$s" > /dev/null || fail "the request does not name the model by its Ollama name, ask for one whole reply, or set num_ctx and the reply's limit per request: $(jq -c '{model,stream,options}' "$s")"
	jq -e '.messages[0] == {role:"system",content:"You are the mechanic."}' "$s" > /dev/null || fail "the system prompt is not the first message: $(jq -c '.messages[0]' "$s")"
	jq -e '.messages[2].role == "assistant" and .messages[2].tool_calls[0].function == {name:"reader",arguments:{command:"read pool/PART.gate@2/gate.c 1-20"}}' "$s" > /dev/null || fail "the model's earlier tool call is not handed back as one: $(jq -c '.messages[2]' "$s")"
	jq -e '.messages[3] == {role:"tool",tool_name:"reader",content:"/* gate.c */"}' "$s" > /dev/null || fail "the tool's answer is not handed back as the reader's: $(jq -c '.messages[3]' "$s")"
	jq -e '[.messages[] | select(.role == "user" and .content == "")] | length == 0' "$s" > /dev/null || fail "an empty user message was sent"
	jq -e '.tools == [{type:"function",function:{name:"reader",description:"Ask the reader.",parameters:{type:"object",properties:{command:{type:"string"}},required:["command"]}}}]' "$s" > /dev/null || fail "the reader is not offered as a tool: $(jq -c '.tools' "$s")"
	jq -e '.usage.inputTokens == 321 and .usage.outputTokens == 45' "$r" > /dev/null || fail "the tokens are not Ollama's own counts: $(jq -c '.usage' "$r")"
	jq -e '.output.message.content[0].text == "The number is wrong." and .output.message.content[1].toolUse.name == "reader" and .output.message.content[1].toolUse.input.command == "read pool/PART.gate@2/gate.c 40-60" and (.output.message.content[1].toolUse.toolUseId | length > 0) and .stopReason == "tool_use"' "$r" > /dev/null || fail "the reply is not handed on in the shape the exam reads: $(jq -c '.output' "$r")"
	# a reply cut off at its limit says so, as Bedrock says it: max_tokens (decision 0086)
	printf '{"model":"qwen-test","created_at":"2026-10-27T20:00:00Z","message":{"role":"assistant","content":"","thinking":"I need to look at"},"done":true,"done_reason":"length","prompt_eval_count":500,"eval_count":2000}\n' > "$tmp/cut.json"
	OLLAMA_FAKE="$tmp/cut.json" sh jig/model converse "$mfx/ollama/req.json" "$tmp/cutr.json" > "$tmp/out" 2>&1 || fail "a cut-off reply was refused: $(head -1 "$tmp/out")"
	jq -e '.stopReason == "max_tokens" and .usage.outputTokens == 2000 and (.output.message.content | length) == 0' "$tmp/cutr.json" > /dev/null || fail "a reply cut off at its limit is not handed on as max_tokens with its thinking counted and not kept: $(jq -c '{stopReason,usage,output}' "$tmp/cutr.json")"
	# stopped short of its limit, the conversation filled the context instead (the 3090's first try, decision 0090)
	printf '{"model":"qwen-test","created_at":"2026-10-27T21:40:00Z","message":{"role":"assistant","content":""},"done":true,"done_reason":"length","prompt_eval_count":49000,"eval_count":150}\n' > "$tmp/full.json"
	OLLAMA_FAKE="$tmp/full.json" sh jig/model converse "$mfx/ollama/req.json" "$tmp/fullr.json" > "$tmp/out" 2>&1 || fail "a reply stopped by a full context was refused: $(head -1 "$tmp/out")"
	jq -e '.stopReason == "model_context_window_exceeded"' "$tmp/fullr.json" > /dev/null || fail "a reply stopped well short of its limit, by a full context, is not handed on as model_context_window_exceeded: $(jq -c '.stopReason' "$tmp/fullr.json")"
	rm -f "$s"; refuse "a local model with no price" "no price for ollama:none" env MODEL_NAME=ollama:none sh jig/model converse "$mfx/ollama/req.json" "$tmp/x.json"
	[ ! -e "$s" ] || fail "a local model with no price was called anyway"
	env -u OLLAMA_FAKE OLLAMA_URL=http://127.0.0.1:9 sh jig/model converse "$mfx/ollama/req.json" "$tmp/x.json" > "$tmp/out" 2>&1; rc=$?
	[ $rc -eq 2 ] && grep -q "could not reach Ollama" "$tmp/out" || fail "an Ollama nobody is listening for gave exit $rc, not 'cannot run here': $(head -1 "$tmp/out")"
	echo "a Bedrock-shaped request reached Ollama as one: its model, num_ctx 32768 and the reply's limit set per request, the system prompt first, the earlier tool call and the reader's answer handed back, the reader offered as a tool; the reply came back in the shape the exam reads, with Ollama's own 321 in and 45 out; a reply cut off at its limit came back as max_tokens, its thinking counted and not kept, and one stopped short of it by a full context as model_context_window_exceeded; a model with no price was refused before any call, and an Ollama nobody listens for is 'cannot run here'" ;;
*)	fail "tests/programs/exam.sh needs a mode: count, record, budget, scorecard, judge, sit, cache, ping or ollama" ;;
esac
