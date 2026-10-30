#!/bin/sh
# Probe for the layered agent's tests (findings 0027).  Builds agent/ with cargo,
# with the lockfile, warnings as errors, with the fake transport
# (the runtime's `fake` feature), and runs one piece of it against fixtures, from
# the outside, through its commands.  Nothing calls a model and nothing boots.
#
#   sh tests/programs/agent.sh runtime TST.agent.runtime@1: the runtime's own tests
#   sh tests/programs/agent.sh note    TST.agent.note@1: the result note
#   sh tests/programs/agent.sh plan    TST.agent.plan@1: the part plan
#   sh tests/programs/agent.sh chat    TST.agent.chat@1: one call to the model
#   sh tests/programs/agent.sh build   TST.agent.build@1: one builder, its fix judged
#   sh tests/programs/agent.sh limit   TST.agent.limit@1: a builder's limits; what is on disk handed in
#   sh tests/programs/agent.sh tokens  TST.agent.tokens@1: the estimate, calibrated by the model's count
#   sh tests/programs/agent.sh trial   TST.agent.trial@1: the second prompts, and the trial against the first
#   sh tests/programs/agent.sh columns TST.agent.columns@1: the race's columns, in every note and on the card
#   sh tests/programs/agent.sh lift    TST.agent.lift@1: an exam item lifted, as jig/sit sits it
#   sh tests/programs/agent.sh direct  TST.agent.direct@1: a director and its limits
#   sh tests/programs/agent.sh bedrock TST.agent.bedrock@1: gear 2, and what it may spend
#
# Exit 0 green, 1 red, 2 where there is no cargo: the agent is built and tested in
# WSL beside Ollama, and the jig has no Rust toolchain (design.md's tool table).
# CARGO_TARGET_DIR (default ~/.cache/aios/agent) keeps the builds between runs.
set -u
cd "$(dirname "$0")/../.."
fail() { echo "$*"; exit 1; }
command -v cargo > /dev/null || { echo "no cargo here: the agent is built and tested in WSL, not on the jig"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
[ -f agent/Cargo.toml ] || fail "the agent does not build: there is no agent/Cargo.toml"
CARGO_TARGET_DIR=${CARGO_TARGET_DIR:-$HOME/.cache/aios/agent}; export CARGO_TARGET_DIR
if [ "${1:-}" = runtime ]; then
	RUSTFLAGS="-D warnings" cargo test -q --locked --features fake --manifest-path agent/Cargo.toml -p runtime > "$t/out" 2>&1 ||
		fail "the runtime's own tests: $(grep -E '^(test .* FAILED|error)' "$t/out" | head -3 | tr '\n' ' ')$(tail -2 "$t/out" | tr '\n' ' ')"
	echo "the runtime's own tests: $(grep -o '[0-9]* passed; 0 failed' "$t/out" | head -1), on saved replies and AWS's published signing examples"
	exit 0
fi
RUSTFLAGS="-D warnings" cargo build -q --locked --features fake --manifest-path agent/Cargo.toml 2> "$t/cc" || fail "the agent does not build: $(grep -m1 -E '^error' "$t/cc")$(head -c 200 "$t/cc" | tr '\n' ' ')"
cp "$CARGO_TARGET_DIR/debug/agent" "$t/agent" || fail "the agent built, but there is no agent at $CARGO_TARGET_DIR/debug/agent"
fx=tests/fixtures/agent; ex=tests/expected

# the toy repo, the fake ssh and the job runner the builder's tests share
toy() {
	rt() { sed -n "s/^$1 *= *//p" "$o/run.toml"; }	# a field of the run being checked
	command -v jq > /dev/null && command -v git > /dev/null || fail "no jq or git here"
	# a toy repo, made here, as the jig's clone and the builder's both: one part whose
	# answer is 41 and should be 42, a test that says so, one that goes red if the fix
	# says "break other", one red before and after, and a jig/test that runs the three
	here=$(pwd); h="$t/home"; a="$h/aios"; mkdir -p "$a/pool/PART.toy@1" "$a/tests" "$a/specs" "$a/jig" "$a/prompts" "$a/runs"
	printf 'int answer = 41;\n' > "$a/pool/PART.toy@1/toy.c"
	awk 'BEGIN { for (i = 1; i <= 500; i++) printf "note %03d: the answer was 41 in the first draft, kept for now\n", i }' > "$a/pool/PART.toy@1/notes.txt"
	printf 'code       = "TST.toy.answer@1"\nbinds      = "SPEC.toy.1@1"\n' > "$a/tests/toy.answer@1.toml"
	printf 'code       = "SPEC.toy.1@1"\nclause     = "The answer is \\"42\\", not 41."\n' > "$a/specs/toy.1@1.toml"
	printf 'You are a builder for the toy.\n' > "$a/prompts/builder@1.txt"
	printf 'a run of an earlier try\n' > "$a/runs/old.txt"
	cat > "$a/jig/test" <<'TOY'
#!/bin/sh
f=pool/PART.toy@1/toy.c; p=0; n=0
if grep -q 'answer = 42' $f; then echo "PASS  TST.toy.answer@1  0s  42"; p=$((p+1)); else echo "FAIL  TST.toy.answer@1  0s  the answer is $(sed 's/[^0-9]//g' $f), not 42"; n=$((n+1)); fi
grep -q forge $f && echo "PASS  TST.toy.answer@1  0s  a line the fix printed itself"
if grep -q 'break other' $f; then echo "FAIL  TST.toy.other@1  0s  broken by the fix"; n=$((n+1))
elif grep -q 'skip other' $f; then echo "SKIP  TST.toy.other@1  0s  skipped"
else echo "PASS  TST.toy.other@1  0s  fine"; p=$((p+1)); fi
[ -d runs ] && { echo "FAIL  TST.toy.copy@1  0s  the runs folder is in the copy"; n=$((n+1)); }
echo "FAIL  TST.toy.old@1  0s  red before any of this"; n=$((n+1))
echo "# $p passed, $n failed, 0 skipped"
TOY
	(cd "$a" && git init -q && git add -A && git -c user.name=t -c user.email=t@t commit -qm toy) || fail "could not make the toy repo"
	at=$(git -C "$a" rev-parse --short=9 HEAD)
	# the fake ssh runs the jig's half here, with the toy home as the jig's home
	# with JIGDOWN set to a file, the jig answers once, then is gone
	printf '#!/bin/sh\nshift\nif [ -n "${JIGDOWN:-}" ]; then [ -f "$JIGDOWN" ] && { echo "ssh: connect to host jig port 22: Connection refused" >&2; exit 255; }; : > "$JIGDOWN"; fi\nHOME=%s exec sh -c "$1"\n' "$h" > "$t/ssh"
	job() {	# job FILE [EXTRA-LINE [TRY]]: the job to fix toy.answer at the toy's commit
		printf 'job = "toy"\ntest = "toy.answer@1"\nat = "%s"\ntry = %s\nprompt = "builder@1"\nmodel = "%s"\nnum_ctx = %s\nroom_share = 50\n%s\n' "$at" "${3:-1}" "${JOBMODEL:-qwen3.8:27b}" "${JOBCTX:-2048}" "${2:-}" > "$1"
	}
	run() {	# run CASE [EXTRA-LINE [TRY]]: the builder on the job, the replies in $t/CASE
		job "$t/$1.toml" "${2:-}" "${3:-1}"
		(cd "$a" && AGENT_FAKE="$t/$1" AGENT_SSH="sh $t/ssh" AGENT_JIG=toy@jig AGENT_OUT="$t/out" AGENT_QUIET=1 \
			AGENT_RUNS="sh $here/jig/runs" RUNS="$t/runs" PRICES="$here/jig/prices.toml" AGENT_SITTING=20261029T120000Z \
			"$t/agent" build "$t/$1.toml" > "$t/$1.out" 2> "$t/$1.err"); rc=$?
	}
}

case "${1:-}" in
note)
	# a real red run on the jig (share.waits red at 3dafa0d before its fix was pulled, 121
	# passed, 1 failed) and a real green one (122 passed); the job's own numbers given
	"$t/agent" note waits-1 1 1234 56 pool/PART.loader@9/loader.c < "$fx/suite-waits-red.txt" > "$t/red.toml" 2> "$t/err" || fail "the note for a red run was not written: $(head -1 "$t/err")"
	cmp -s "$t/red.toml" "$ex/agent.note-red.toml" || fail "the note for a red run is not the one expected: $(diff "$ex/agent.note-red.toml" "$t/red.toml" | head -4 | tr '\n' ' ')"
	"$t/agent" note waits-2 2 98765 412 pool/PART.loader@9/loader.c < "$fx/suite-waits-green.txt" > "$t/green.toml" 2> "$t/err" || fail "the note for a green run was not written: $(head -1 "$t/err")"
	cmp -s "$t/green.toml" "$ex/agent.note-green.toml" || fail "the note for a green run is not the one expected: $(diff "$ex/agent.note-green.toml" "$t/green.toml" | head -4 | tr '\n' ' ')"
	# made up, to be worse than any real run: 60 failing tests with long messages, 30 files
	awk 'BEGIN { for (i = 1; i <= 60; i++) printf "FAIL  TST.made.up%02d@1  1s  %s\n", i, "this message is far too long to go to the director whole, and it goes on and on and on and on and on and on and on, well past one line of any sensible width at all"
		for (i = 1; i <= 3; i++) printf "PASS  TST.made.fine%d@1  0s  ok\n", i; print "# 3 passed, 60 failed, 0 skipped" }' > "$t/many.txt"
	files=$(awk 'BEGIN { for (i = 1; i <= 30; i++) printf "pool/PART.made@1/f%02d.c ", i }')
	# shellcheck disable=SC2086
	"$t/agent" note many-1 3 10 20 $files < "$t/many.txt" > "$t/many.toml" 2> "$t/err" || fail "the note for a very red run was not written: $(head -1 "$t/err")"
	w=$(wc -w < "$t/many.toml")
	[ "$w" -le 200 ] || fail "the note for 60 failing tests and 30 files is $w words, over the 200 a director is given"
	grep -q '^tests_failed  = 60$' "$t/many.toml" && grep -q '^tests_passed  = 3$' "$t/many.toml" || fail "the counts were cut along with the names: $(grep '^tests_' "$t/many.toml" | tr '\n' ' ')"
	grep -q '^failing_more  = 40$' "$t/many.toml" && grep -q '^files_more    = 10$' "$t/many.toml" || fail "the names and files left out of the note are not counted in it"
	[ "$(sed -n 's/^first_failure = "\(.*\)"$/\1/p' "$t/many.toml" | wc -c)" -le 161 ] || fail "the first failure is not cut to one line of 160 characters"
	# a try that is not a number writes nothing and says why
	"$t/agent" note bad-1 one 0 0 < "$fx/suite-waits-red.txt" > "$t/bad.toml" 2> "$t/err"; rc=$?
	[ $rc -eq 2 ] && [ ! -s "$t/bad.toml" ] && grep -q 'try' "$t/err" || fail "a try that is not a number gave exit $rc and wrote $(wc -c < "$t/bad.toml") bytes"
	echo "a red run and a green run from the jig give the notes expected, word for word; 60 failing tests and 30 files fit in $w words, the 40 and 10 left out counted, the first failure cut to a line; a try that is not a number writes nothing" ;;
plan)
	"$t/agent" plan "$fx/plan-waits.toml" > "$t/plan.toml" 2> "$t/err" || fail "a good part plan was refused: $(head -1 "$t/err")"
	cmp -s "$t/plan.toml" "$ex/agent.plan-waits.toml" || fail "the plan read back is not the one expected, every default filled in: $(diff "$ex/agent.plan-waits.toml" "$t/plan.toml" | head -4 | tr '\n' ' ')"
	# each spoiled copy refused, naming the field, and nothing printed
	n=0
	spoil() {	# spoil NAME SED-EXPRESSION WORD-THE-REFUSAL-NAMES
		n=$((n + 1)); sed "$2" "$fx/plan-waits.toml" > "$t/spoiled$n.toml"	# a name the refusal cannot match by accident
		"$t/agent" plan "$t/spoiled$n.toml" > "$t/out" 2> "$t/err"; rc=$?
		[ $rc -eq 1 ] && [ ! -s "$t/out" ] && grep -q "$3" "$t/err" || fail "a plan with $1 gave exit $rc, printed $(wc -c < "$t/out") bytes, and said: $(head -1 "$t/err")"
	}
	spoil "no tests" '/^tests/d' 'tests'
	spoil "an empty test list" 's/^tests .*/tests = []/' 'tests'
	spoil "a third gear" 's/^tactical = 1/tactical = 3/' 'gear.tactical'
	spoil "a key no plan has" 's/^budget_usd = 0/budget_usd = 0\nmax_tokens = 9/' 'max_tokens'
	spoil "a commit that is not hex" 's/9c6c852/main/' 'at'
	spoil "a prompt with no version" 's/director@1/director/' 'prompts'
	spoil "a try cap as a word" 's/^budget_usd = 0/budget_usd = 0\nmax_tries = "ten"/' 'max_tries'
	spoil "no try at all" 's/^budget_usd = 0/budget_usd = 0\nmax_tries = 0/' 'max_tries'
	spoil "a string never closed" 's/director@1"/director@1/' 'not closed'
	spoil "a key given twice" 's/^budget_usd = 0/budget_usd = 0\nbudget_usd = 1/' 'twice'
	# the part plans in the repo say their time cap, 240 minutes, not leave it to the default
	for p in agent/plans/*.toml; do grep -qx 'max_minutes = 240' "$p" || fail "$p does not say max_minutes = 240"; done
	echo "a part plan read back with every default filled in: 10 tries, 240 minutes, stuck at 3 of the same or 5 with no drop; refused, naming the field, with no tests, an empty list, a third gear, a key no plan has, a commit that is not hex, a prompt with no version, a try cap as a word or 0, a string never closed, a key given twice; every plan in agent/plans says 240 minutes" ;;
chat)
	command -v jq > /dev/null || fail "no jq here to read what the agent sent"
	# replies made up in Ollama's shape, one per way a call ends; each case gets its own
	# folder, as the fake hands back 1.json and keeps what was sent as sent-1.json
	call() {	# call REPLY: one chat with REPLY as the model's answer; $rc, $t/REPLY/{out,err}
		mkdir "$t/$1"; [ -f "$fx/chat/$1.json" ] && cp "$fx/chat/$1.json" "$t/$1/1.json"
		AGENT_FAKE="$t/$1" OLLAMA_URL=http://ollama.test:11434 "$t/agent" chat qwen3.8:27b 98304 2000 \
			< "$fx/chat/conversation.json" > "$t/$1/out" 2> "$t/$1/err"; rc=$?
	}
	field() { sed -n "s/^$2 *= //p" "$t/$1/out"; }
	call tool
	[ $rc -eq 0 ] || fail "a reply asking for a tool gave exit $rc: $(head -1 "$t/tool/err")"
	s="$t/tool/sent-1.json"
	[ "$(jq -r '.model, .stream, .options.num_ctx, .options.num_predict' "$s" | tr '\n' ' ')" = "qwen3.8:27b true 98304 2000 " ] ||
		fail "the model, stream, context and reply limit sent are not the ones asked for: $(jq -c '{model, stream, options}' "$s")"
	[ "$(jq -r '.messages[0].role, .messages[0].content' "$s" | tr '\n' '|')" = "system|You are a builder.  One job, then submit.|" ] ||
		fail "the system prompt is not sent first, as its own message: $(jq -c '.messages[0]' "$s")"
	[ "$(jq -c '.messages[1:]' "$s")" = "$(jq -c '.messages' "$fx/chat/conversation.json")" ] ||
		fail "the conversation is not sent as it was given, in order, after the system prompt"
	[ "$(jq -c '.tools' "$s")" = "$(jq -c '.tools' "$fx/chat/conversation.json")" ] || fail "the tools are not sent as given"
	[ "$(field tool stop)" = '"tool"' ] && [ "$(field tool tokens_in)" = 5321 ] && [ "$(field tool tokens_out)" = 412 ] ||
		fail "a reply asking for a tool is not read as one, with its tokens: $(head -3 "$t/tool/out" | tr '\n' ' ')"
	m=$(field tool message)
	[ "$(echo "$m" | jq -r '.tool_calls[0].function.arguments.why')" = "see where the interrupt flag is set" ] ||
		fail "the tool call, and its why, are not in the reply kept: $m"
	[ "$(echo "$m" | jq 'has("thinking")')" = false ] || fail "the thinking is kept in the conversation, and fills it"
	call end
	[ $rc -eq 0 ] && [ "$(field end stop)" = '"end"' ] || fail "a reply that just says something is read as $(field end stop), exit $rc"
	call cut
	[ $rc -eq 0 ] && [ "$(field cut stop)" = '"max_tokens"' ] || fail "a reply cut off at its own 2000 tokens is read as $(field cut stop), exit $rc"
	call full
	[ $rc -eq 0 ] && [ "$(field full stop)" = '"context"' ] || fail "a reply cut off at 204 tokens, the context full, is read as $(field full stop), exit $rc"
	call garbled
	[ $rc -eq 1 ] && [ "$(field garbled stop)" = '"error"' ] && grep -q 'could not be read' "$t/garbled/err" ||
		fail "a reply that is not Ollama's gave exit $rc, read as $(field garbled stop), and said: $(head -1 "$t/garbled/err")"
	call gone
	[ $rc -eq 1 ] && [ "$(field gone stop)" = '"unreachable"' ] && grep -q 'ollama.test:11434' "$t/gone/err" ||
		fail "no model there gave exit $rc, read as $(field gone stop), and said: $(head -1 "$t/gone/err")"
	echo "the model, context, reply limit, system prompt first, the conversation and tools sent as given; a tool call read with its why and tokens, its thinking dropped; a plain answer, a cut at the reply limit and a full context told apart; a reply that is not Ollama's and no model at all fail, saying so" ;;
build)
	toy
	mkdir "$t/whole"; cp "$fx"/build/[0-9].json "$t/whole/"
	run whole
	o="$t/out/toy-try1"
	[ $rc -eq 0 ] || fail "a builder that hands in the right fix on its eighth call gave exit $rc: $(head -2 "$t/whole.err" | tr '\n' ' ')$(cat "$t/whole.out")"
	rt() { sed -n "s/^$1 *= *//p" "$o/run.toml"; }
	[ "$(rt result)" = '"pass"' ] && [ "$(rt ended)" = '"pass"' ] && [ "$(rt calls)" = 8 ] && [ "$(rt hand_ins)" = 2 ] ||
		fail "the run is not recorded as a pass on call 8 after 2 hand-ins judged (the refused one is not run): $(grep -E '^(result|ended|calls|hand_ins)' "$o/run.toml" | tr '\n' ' ')"
	[ "$(rt bench)" = '"agent"' ] && [ "$(rt peak_context)" = 2250 ] && [ "$(rt tokens_in)" = 11910 ] && [ "$(rt tokens_out)" = 1070 ] ||
		fail "the bench, the peak context (2250, the biggest call) or the tokens are not recorded: $(grep -E '^(bench|peak_context|tokens_)' "$o/run.toml" | tr '\n' ' ')"
	# what the builder was sent: the role in the system prompt, the one job in the first message
	s1="$t/whole/sent-1.json"
	[ "$(jq -r '.messages[0].content' "$s1")" = "You are a builder for the toy." ] || fail "the system prompt is not the library's builder@1"
	jq -r '.messages[1].content' "$s1" > "$t/first"
	grep -q 'The answer is "42", not 41.' "$t/first" && grep -q '^FAIL  TST.toy.answer@1  0s  the answer is 41, not 42$' "$t/first" ||
		fail "the first message does not hold the clause and what the red test printed on the jig: $(head -c 300 "$t/first" | tr '\n' ' ')"
	ans() { jq -r "[.messages[] | select(.role == \"tool\")][$2].content" "$t/whole/sent-$1.json"; }
	ans 2 0 | grep -q '^refused: say why' || fail "a look with no why was run: $(ans 2 0 | head -1)"
	ans 3 1 | grep -q "^1	int answer = 41;$" || fail "a look that said why was not answered from the copy: $(ans 3 1 | head -2 | tr '\n' ' ')"
	ans 4 2 | grep -q '^… truncated, [0-9]* more characters. Narrow the query.$' || fail "400 lines of notes were not cut with the marker: $(ans 4 2 | tail -1)"
	[ "$(ans 4 2 | head -c 8000 | wc -c)" -eq 8000 ] || fail "the cut answer is not 8000 characters before its marker"
	ans 5 3 | grep -q '^refused: ../../etc/passwd is outside the repo' || fail "a look outside the copy was not refused: $(ans 5 3)"
	ans 6 4 | grep -q 'could not be read' || fail "a call whose arguments are not JSON was not answered with what was wrong: $(ans 6 4)"
	ans 7 5 | grep -q 'judged: fail.*1 newly red: FAIL  TST.toy.other@1' || fail "a fix that broke a green test was not shown what it broke: $(ans 7 5 | head -2 | tr '\n' ' ')"
	ans 7 5 | grep -q 'toy.old' && fail "a test red before the fix was counted against it"
	ans 8 6 | grep -q 'refused: only a file under pool/' || fail "a fix to the test itself was not refused: $(ans 8 6)"
	jq -r '.messages[-1].content' "$t/whole/sent-2.json" | grep -q '^Call 1 of 60 done; 59 left.$' || fail "the builder is not told how many calls it has left"
	jq -e '[.messages[] | select(.thinking)] | length == 0' "$t/whole/sent-8.json" > /dev/null || fail "a thinking was kept in the conversation"
	# 2048 tokens of context and half of it for the conversation: the 8000 characters of
	# notes are dropped once four newer messages are in, and the builder is told so
	ans 8 2 | grep -q '^\[dropped to make room: this answer was 80[0-9][0-9] characters; look again if you need it\]$' ||
		fail "an old answer was not dropped to make room, nor said so: $(ans 8 2 | head -c 120)"
	[ "$(jq '[.messages[-4:][] | select(.role == "tool") | select(.content | startswith("[dropped"))] | length' "$t/whole/sent-8.json")" -eq 0 ] &&
		[ "$(jq '[.messages[-4:][] | select(.role == "tool")] | length' "$t/whole/sent-8.json")" -ge 1 ] || fail "an answer in the last four messages was dropped; they are kept word for word"
	[ "$(sed -n 's/^status *= *//p' "$o/note.toml")" = '"fail"' ] && grep -q '^tests_failed  = 1$' "$o/note.toml" && grep -q '"pool/PART.toy@1/toy.c"' "$o/note.toml" ||
		fail "the note from the last suite run is not the one the jig printed: $(tr '\n' ' ' < "$o/note.toml" | cut -c1-200)"
	grep -q '^=== pool/PART.toy@1/toy.c$' "$o/output.txt" && [ "$(rt lines)" = 2 ] || fail "the change is not kept with the lines it changed: $(rt lines)"
	grep -q 'toy.copy' "$o/before.txt" "$o/suite.txt" && fail "the jig's copy of the repo has runs/ in it, so an earlier run is there to read"
	[ -d "$o/base/pool" ] && [ ! -e "$o/base/runs" ] || fail "the builder's copy is missing, or has runs/ in it"
	# recorded in runs/ as every run is, so the month's spend and the card count it
	rr=$(ls "$t"/runs/*/run.toml 2> /dev/null)
	[ "$(echo "$rr" | grep -c .)" -eq 1 ] || fail "$(echo "$rr" | grep -c .) runs recorded in runs/, not 1"
	for want in 'item          = "fix-toy-answer"' 'model         = "ollama:qwen3.8:27b"' 'sitting       = "20261029T120000Z"' 'result        = "pass"' \
		'lines         = 2' 'input_tokens  = 11910' 'bench         = "agent"' 'peak_context  = 2250' 'ended         = "pass"' "at            = \"$at\""; do
		grep -qxF "$want" "$rr" || fail "the run in runs/ does not say $want: $(grep -E '^(item|result|bench|peak|ended)' "$rr" | tr '\n' ' ')"
	done
	git -C "$a" diff --quiet HEAD && [ -z "$(git -C "$a" status --porcelain)" ] || fail "the builder touched the repo it was given: $(git -C "$a" status --porcelain | head -2)"
	# out of room: a reply cut off short of its limit ends the run, honestly; nothing was
	# changed, and the tests still run, so the director still gets a note
	mkdir "$t/room"; cp "$fx/build/room.json" "$t/room/1.json"
	run room '' 2
	o="$t/out/toy-try2"
	[ $rc -eq 1 ] && [ "$(rt ended)" = '"room"' ] && [ "$(rt result)" = '"fail"' ] && [ "$(rt peak_context)" = 97204 ] &&
		grep -q '^tests_failed  = 2$' "$o/note.toml" &&
		grep -qx 'ended         = "room"' "$t"/runs/*/run.toml ||
		fail "a builder whose conversation filled the card gave exit $rc and was recorded $(grep -E '^(ended|result|peak_context)' "$o/run.toml" | tr '\n' ' ')"
	# no model at all on the first call: cannot run, and nothing recorded as a try
	mkdir "$t/none"; run none "" 3; o="$t/out/toy-try3"
	[ $rc -eq 2 ] && [ ! -f "$o/run.toml" ] && [ "$(ls "$t/runs" | wc -l)" -eq 2 ] || fail "a builder with no model to reach gave exit $rc, or recorded a try"
	# a job with a key no job has: refused, naming it, before anything runs
	mkdir "$t/odd"; run odd 'max_turn = 9'
	[ $rc -eq 2 ] && grep -q 'max_turn' "$t/odd.err" && [ ! -f "$t/odd/sent-1.json" ] || fail "a job with a key no job has gave exit $rc and said: $(head -1 "$t/odd.err")"
	# a hand-in's paths are checked before anything is written: none may climb out of the
	# folder, and none may name a part by way of "."
	mkdir "$t/escape"; cp "$fx/build/escape-1.json" "$t/escape/1.json"; cp "$fx/build/escape-2.json" "$t/escape/2.json"
	run escape '' 6
	[ ! -e "$t/escaped.txt" ] && [ ! -e "$t/out/escaped.txt" ] || fail "a hand-in path with .. wrote a file outside the builder's folder"
	ans() { jq -r "[.messages[] | select(.role == \"tool\")][$2].content" "$t/$1/sent-$3.json"; }
	ans escape 0 2 | grep -q 'refused: only a file under pool/ that already exists may be fixed, not ../../../escaped.txt' ||
		fail "a hand-in path with .. was not refused: $(ans escape 0 2)"
	grep -q 'refused' "$t/escape/3.json" 2> /dev/null; [ -f "$t/escape/sent-3.json" ] && ans escape 1 3 | grep -q 'not pool/./PART.toy@1/toy.c' ||
		fail "a hand-in path through . was not refused"
	# a fix that stops a green test is not a pass, nor one that prints its own PASS beside the FAIL
	mkdir "$t/skip"; cp "$fx/build/skip-1.json" "$t/skip/1.json"; run skip '' 8
	ans skip 0 2 | grep -q 'judged: fail.*1 newly red: GONE  TST.toy.other@1  green before, now skipped or not run' ||
		fail "a fix that made a green test skip was not failed for it: $(ans skip 0 2 | head -1)"
	mkdir "$t/forge"; cp "$fx/build/forge-1.json" "$t/forge/1.json"; run forge '' 9
	ans forge 0 2 | grep -q 'judged: fail.  the test it was given is still red' ||
		fail "a fix whose test printed both PASS and FAIL was taken: $(ans forge 0 2 | head -1)"
	# the jig gone between the suite before and the hand-in: not the builder's failure, so
	# cannot run, recorded, and the note is the suite before, not an ssh error with no counts
	mkdir "$t/down"; cp "$fx/build/8.json" "$t/down/1.json"; JIGDOWN="$t/jigdown" run down '' 10
	o="$t/out/toy-try10"
	[ $rc -eq 2 ] && [ "$(rt ended)" = '"error"' ] && grep -q 'did not run on the jig' "$o/run.toml" && grep -q '^tests_failed  = 2$' "$o/note.toml" ||
		fail "a jig gone at the hand-in gave exit $rc, ended $(rt ended), and a note of $(grep tests_failed "$o/note.toml")"
	# the out folder given as a path from here, as it is by default: the judge still finds it
	mkdir "$t/relout"; cp "$fx/build/8.json" "$t/relout/1.json"; job "$t/relout.toml" '' 7
	(cd "$a" && AGENT_FAKE="$t/relout" AGENT_SSH="sh $t/ssh" AGENT_JIG=toy@jig AGENT_OUT=out/agent AGENT_QUIET=1 \
		AGENT_RUNS="sh $here/jig/runs" RUNS="$t/runs" PRICES="$here/jig/prices.toml" "$t/agent" build "$t/relout.toml" > "$t/relout.out" 2> "$t/relout.err"); rc=$?
	[ $rc -eq 0 ] && grep -q '^result       = "pass"$' "$a/out/agent/toy-try7/run.toml" ||
		fail "the right fix with the out folder given from here was not a pass: exit $rc, $(cat "$t/relout.err")$(cat "$t/relout.out")"
	# gear 2: no call past the month's cap, and none past the job's share of the part's
	cp "$fx/bedrock/aws" "$t/aws"; chmod 600 "$t/aws"; printf 'monthly_cap_usd = 0.10\n' > "$t/cap.toml"
	export AIOS_AWS="$t/aws" JOBMODEL=global.anthropic.claude-sonnet-4-6
	mkdir "$t/capped"; cp "$fx/bedrock/tool.json" "$t/capped/1.json"
	BUDGET="$t/cap.toml" run capped 'gear = 2' 4
	[ $rc -eq 2 ] && grep -q 'would pass the cap of 0.10' "$t/capped.err" && [ ! -f "$t/capped/sent-1.json" ] ||
		fail "a paid call the month's cap had no room for gave exit $rc, said $(head -1 "$t/capped.err"), and was sent: $(ls "$t/capped" | grep -c sent)"
	mkdir "$t/share"; cp "$fx/bedrock/tool.json" "$t/share/1.json"; cp "$fx/bedrock/tool.json" "$t/share/2.json"
	AGENT_MAX_COST=0.01 run share "$(printf 'gear = 2\nmax_usd = 0.012')" 5
	o="$t/out/toy-try5"
	[ $rc -eq 1 ] && [ -f "$t/share/sent-1.json" ] && [ ! -f "$t/share/sent-2.json" ] && [ "$(rt ended)" = '"budget"' ] && [ "$(rt calls)" = 1 ] &&
		[ "$(rt dollars)" = 0.004500 ] && [ "$(rt model)" = '"bedrock:global.anthropic.claude-sonnet-4-6"' ] ||
		fail "a job with \$0.012 to spend, \$0.0045 spent and a call that may cost \$0.01 made it, or was not recorded as ended on the budget: exit $rc, $(grep -E '^(ended|calls|dollars|model)' "$o/run.toml" | tr '\n' ' ')"
	grep -qx 'ended         = "budget"' "$t"/runs/*-toy-answer-5-*/run.toml && grep -qx 'cost_usd      = 0.004500' "$t"/runs/*-toy-answer-5-*/run.toml ||
		fail "the paid run is not in runs/ with its cost"
	unset AIOS_AWS JOBMODEL
	echo "a builder on a toy repo: a look with no why refused, one with a why answered from its copy, 400 lines cut at 8000 characters with the marker, a look outside refused, a call it could not read answered, a fix that broke a green test shown what it broke and not blamed for one red before, a fix to the test refused, a fix that makes a green test skip or prints its own PASS beside the FAIL failed, a hand-in path climbing out or through . refused before anything is written, the right fix a pass on call 8, also with the out folder given from here; a jig gone at the hand-in is cannot-run, with the note from the suite before; told its calls left, thinking dropped, an old answer dropped to make room and the last four kept; peak context, tokens, the note and the change recorded, and the run in runs/ with its bench and ending; the repo untouched; a full card ends as room, no model is cannot-run, and a job with a key no job has is refused; on gear 2 a call the month's cap has no room for is not sent, and a job stops before a call that could pass its share of the part's dollars, recorded with its cost" ;;
trial)
	pr=prompts
	# the old versions are kept as they were used on waits (decision 0096's run)
	[ "$(git hash-object "$pr/director@1.txt" | cut -c1-8)" = 1b06a8df ] && [ "$(git hash-object "$pr/builder@1.txt" | cut -c1-8)" = cf2c3ca3 ] ||
		fail "director@1 or builder@1 changed: a version is never written over"
	grep -q 'Every builder starts from a clean checkout of the plan.s commit' "$pr/director@2.txt" &&
		grep -q 'Never describe a file as already changed' "$pr/director@2.txt" ||
		fail "director@2 does not say every builder starts from a clean checkout, never describing a file as already changed"
	grep -q 'A hint must say something the result note did not already say' "$pr/director@2.txt" && grep -q 'If you have nothing to add, give no hint' "$pr/director@2.txt" ||
		fail "director@2 does not say a hint must add to the note, or be left out"
	grep -q "Read the test's source first, before anything else" "$pr/builder@2.txt" && grep -q 'Do not guess at what a test wants' "$pr/builder@2.txt" ||
		fail "builder@2 does not hold reading the test's source first as a standing rule"
	grep -q 'hands in nothing' "$pr/builder@2.txt" && fail "builder@2 still says a run can hand in nothing"
	# the trial: two plans alike but for their prompts, under one trial name
	for v in old new; do "$t/agent" plan agent/plans/waits-$v.toml > "$t/$v.toml" 2> "$t/$v.err" || fail "the $v trial plan was refused: $(cat "$t/$v.err")"; done
	grep -q '^trial  *= "prompts-2"$' "$t/old.toml" && grep -q '^director  *= "director@1"$' "$t/old.toml" && grep -q '^director  *= "director@2"$' "$t/new.toml" ||
		fail "the trial plans do not name trial prompts-2 with the old and new prompts"
	[ "$(grep -v -E '^(director|builder) ' "$t/old.toml")" = "$(grep -v -E '^(director|builder) ' "$t/new.toml")" ] ||
		fail "the trial plans differ in more than their prompts: $(diff "$t/old.toml" "$t/new.toml" | head -4 | tr '\n' ' ')"
	# a director on a trial plan: the plan it is shown has no comments, its jobs carry the
	# trial, and its run and its builders' are recorded under it
	cat > "$t/builder" <<'FAKE'
#!/bin/sh
jf=$2; job=$(sed -n 's/^job = "\(.*\)"$/\1/p' "$jf"); try=$(sed -n 's/^try = //p' "$jf"); cp "$jf" "$T/seen-$job.toml"
o=$AGENT_OUT/$job-try$try; mkdir -p "$o"
printf 'PASS  TST.share.waits@1  1s  ok\n# 1 passed, 0 failed, 0 skipped\n' | "$T/agent" note "$job" "$try" 10 10 > "$o/note.toml"
printf 'result       = "pass"\nended        = "pass"\nwhy          = ""\n' > "$o/run.toml"
FAKE
	mkdir "$t/fake"; cp "$fx/direct/done/4.json" "$t/fake/1.json"
	printf '{"model":"qwen3.8:27b","message":{"role":"assistant","content":"","tool_calls":[{"function":{"name":"done","arguments":{"report":"Passed."}}}]},"done":true,"done_reason":"stop","prompt_eval_count":900,"eval_count":20}\n' > "$t/fake/2.json"
	sed 's/toy.answer@1/share.waits@1/' "$t/fake/1.json" > "$t/fake/x" && mv "$t/fake/x" "$t/fake/1.json"
	cp agent/plans/waits-new.toml "$t/waits-new.toml"
	T=$t AGENT_FAKE="$t/fake" AGENT_BUILDER="sh $t/builder" AGENT_OUT="$t/out" AGENT_QUIET=1 AGENT_RUNS="sh $(pwd)/jig/runs" RUNS="$t/runs" PRICES="$(pwd)/jig/prices.toml" \
		"$t/agent" direct "$t/waits-new.toml" > "$t/d.out" 2> "$t/d.err"; rc=$?
	[ $rc -eq 0 ] || fail "a director on the trial plan gave exit $rc: $(cat "$t/d.err" "$t/d.out" | head -3)"
	jq -r '.messages[1].content' "$t/fake/sent-1.json" | grep -q '^#' && fail "the director was shown the plan's comments"
	grep -qx 'trial = "prompts-2"' "$t/seen-waits-new-1.toml" || fail "the builder's job does not carry the trial"
	grep -qx 'trial         = "prompts-2"' "$t"/runs/*-direct-waits-new-*/run.toml || fail "the director's run is not recorded under the trial"
	# the card: the trial's two runs shown together, marked, each with its director's prompt
	RUNS_TRIAL=prompts-2 RUNS_BENCH=agent RUNS_ENDED=stuck RUNS_PEAK=3000 RUNS_PROMPT=1b06a8df RUNS_SITTING=20261030T090000Z RUNS="$t/runs" PRICES="$(pwd)/jig/prices.toml" \
		sh jig/runs record direct-waits-old ollama:qwen3.8:27b 2026-10-30 1 100 10 fail 0 "$t/d.out" "$t/d.out" > /dev/null || fail "the old run could not be recorded"
	RUNS="$t/runs" sh jig/scorecard > "$t/card" 2>&1 || fail "the scorecard failed: $(head -2 "$t/card")"
	sed -n '/^The layered bench, by part$/,$p' "$t/card" > "$t/parts"
	grep -q '^  prompt trial prompts-2$' "$t/parts" && grep -q '^    waits-new .*PASS  ended pass' "$t/parts" &&
		grep -q '^    waits-old .*director prompt 1b06a8df  FAIL  ended stuck' "$t/parts" ||
		fail "the card does not show the trial's two runs together, marked, each with its prompt and ending: $(cat "$t/parts")"
	echo "director@1 and builder@1 kept as used on waits; director@2 says every builder starts from a clean checkout and a hint must add to the note or be left out; builder@2 reads the test's source first as a standing rule; the two trial plans differ only in their prompts; a director on one is shown the plan with no comments, its jobs and its run carry the trial, and the card shows the trial's two runs together" ;;
tokens)
	tf="$fx/tokens"
	# the model's own count of a file: its count with the file, less its count with nothing
	mkdir "$t/count"; cp "$tf/count-1.json" "$t/count/1.json"; cp "$tf/count-2.json" "$t/count/2.json"
	AGENT_FAKE="$t/count" "$t/agent" count qwen3.8:27b "$tf/check.c" > "$t/count.out" 2> "$t/count.err" || fail "count failed: $(head -1 "$t/count.err")"
	grep -q "check.c  1552 tokens counted by qwen3.8:27b" "$t/count.out" || fail "the count is not 1563 less 11: $(cat "$t/count.out")"
	[ "$(jq '.think' "$t/count/sent-1.json")" = false ] && [ "$(jq '.options.num_predict' "$t/count/sent-1.json")" = 1 ] ||
		fail "counting asked the model to think or to say more than one token"
	# in a builder: once the model's count says the conversation is past its share, old
	# answers are dropped, though characters over four says there is room
	toy
	for c in heavy light; do mkdir "$t/$c"; for n in 1 2 3; do cp "$tf/$c-$n.json" "$t/$c/$n.json"; done; done
	JOBCTX=8192 run heavy '' 15; JOBCTX=8192 run light '' 16
	jq -r '[.messages[] | select(.role == "tool")][0].content' "$t/heavy/sent-3.json" | grep -q '^\[dropped to make room' ||
		fail "the model counted 5200 tokens against a share of 4096 and nothing was dropped"
	jq -r '[.messages[] | select(.role == "tool")][0].content' "$t/light/sent-3.json" | grep -q '^\[dropped' &&
		fail "with the model's count under the share, an answer was dropped anyway"
	# a known code block: calibrated on another, the estimate is within 10 percent of what
	# the model counts; characters over four alone is not (both counts measured on the card)
	[ -f "$tf/counted.txt" ] || fail "no counts measured on the card yet in $tf/counted.txt"
	cal=$(sed -n 's/^calibrate.c  *\([0-9]*\) .*/\1/p' "$tf/counted.txt"); chk=$(sed -n 's/^check.c  *\([0-9]*\) .*/\1/p' "$tf/counted.txt")
	[ -n "$cal" ] && [ -n "$chk" ] || fail "counted.txt does not give both files' counts"
	est=$("$t/agent" estimate "$tf/calibrate.c" "$cal" "$tf/check.c") || fail "estimate failed"
	plain=$(( ($(wc -m < "$tf/check.c") + 3) / 4 ))
	awk -v e="$est" -v c="$chk" 'BEGIN { exit !((e - c) * (e - c) <= (c * 0.10) * (c * 0.10)) }' ||
		fail "the calibrated estimate for check.c is $est, more than 10 percent from the $chk the model counted"
	awk -v e="$plain" -v c="$chk" 'BEGIN { exit !((e - c) * (e - c) > (c * 0.10) * (c * 0.10)) }' ||
		fail "characters over four alone, $plain, is within 10 percent of $chk: the test no longer shows the undercount"
	echo "the model's count of a file is its count with it less its count without, asked with no thinking and one token; calibrated on one code block, the estimate for another is $est against the $chk the model counted, within 10 percent, where characters over four says $plain; a builder drops old answers when the model's count says the share is passed, not when the plain estimate does" ;;
limit)
	toy
	# thinking past its cap is stopped there, and the turn asked again with no thinking,
	# so the reply's own limit goes to what it says and does (findings 0029)
	mkdir "$t/think"; cp "$fx/build/think-1.json" "$t/think/1.json"; cp "$fx/build/think-2.json" "$t/think/2.json"
	run think 'think_tokens = 2000' 11
	o="$t/out/toy-try11"
	[ "$(cat "$t/think/sent-1.read")" = 3 ] || fail "the thinking was read past its cap of 2000 tokens: $(cat "$t/think/sent-1.read") of 6 pieces read"
	[ "$(jq '.think' "$t/think/sent-1.json")" = null ] && [ "$(jq '.options.num_predict' "$t/think/sent-1.json")" = 14000 ] ||
		fail "the first ask is not with thinking, and room for 2000 of it on top of the reply's 12000: $(jq -c '{think, options}' "$t/think/sent-1.json")"
	[ "$(jq '.think' "$t/think/sent-2.json")" = false ] && [ "$(jq '.options.num_predict' "$t/think/sent-2.json")" = 12000 ] ||
		fail "the turn was not asked again with thinking off and the reply's own 12000: $(jq -c '{think, options}' "$t/think/sent-2.json")"
	[ "$(jq '.messages | length' "$t/think/sent-2.json")" = "$(jq '.messages | length' "$t/think/sent-1.json")" ] || fail "the second ask is not the same turn"
	# the edit it made then is on disk, and with no more replies the run ends: the program
	# hands it in, and it passes
	[ "$(rt result)" = '"pass"' ] && [ "$(rt handed_in_by)" = '"program"' ] ||
		fail "an edit left on disk when the run ended was not handed in and judged: $(grep -E '^(result|ended|handed_in_by)' "$o/run.toml" | tr '\n' ' ')"
	# a builder whose replies are cut off at their own limit: what it changed on disk is
	# handed in for it, and its test passes
	mkdir "$t/cut"; for n in 1 2 3; do cp "$fx/build/cut-$n.json" "$t/cut/$n.json"; done
	run cut '' 12
	o="$t/out/toy-try12"
	[ $rc -eq 0 ] && [ "$(rt ended)" = '"stopped"' ] && [ "$(rt result)" = '"pass"' ] && [ "$(rt handed_in_by)" = '"program"' ] && [ "$(rt hand_ins)" = 1 ] ||
		fail "a builder cut off after an edit did not have its edit handed in: exit $rc, $(grep -E '^(result|ended|why|handed_in_by|hand_ins)' "$o/run.toml" | tr '\n' ' ')"
	grep -q '^status        = "fail"$' "$o/note.toml" && grep -q '"pool/PART.toy@1/toy.c"' "$o/note.toml" || fail "the note does not come from the program's hand-in"
	# cut off with nothing changed: no builder hands in nothing; the tests run on the commit as
	# it was and there is a note
	mkdir "$t/bare"; cp "$fx/build/bare-1.json" "$t/bare/1.json"; cp "$fx/build/bare-2.json" "$t/bare/2.json"
	run bare '' 13
	o="$t/out/toy-try13"
	[ $rc -eq 1 ] && [ "$(rt result)" = '"fail"' ] && [ "$(rt handed_in_by)" = '"program"' ] && grep -q '^tests_failed  = 2$' "$o/note.toml" &&
		grep -q 'nothing was changed on disk' "$t/bare.out" ||
		fail "a builder cut off with nothing changed did not get its tests run and a note: exit $rc, $(grep -E '^(result|handed_in_by)' "$o/run.toml" | tr '\n' ' '), $(cat "$t/bare.out")"
	# an edit must find its lines exactly once, and only under pool/
	mkdir "$t/badedit"; cp "$fx/build/badedit-1.json" "$t/badedit/1.json"; cp "$fx/build/badedit-2.json" "$t/badedit/2.json"
	run badedit '' 14
	ans3() { jq -r "[.messages[] | select(.role == \"tool\")][$2].content" "$t/$1/sent-$3.json"; }
	ans3 badedit 0 2 | grep -q 'does not hold old exactly as given.  Nothing was changed' || fail "an edit whose lines are not in the file was made: $(ans3 badedit 0 2)"
	ans3 badedit 1 3 | grep -q 'refused: only a file under pool/' || fail "an edit to a test was made: $(ans3 badedit 1 3)"
	grep -qx 'code       = "TST.toy.answer@1"' "$t/out/toy-try14/work/tests/toy.answer@1.toml" || fail "the refused edit changed the file anyway"
	echo "a builder's thinking past its cap of 2000 is stopped after 3 of 6 pieces and the turn asked again with thinking off and the reply's own 12000; an edit left on disk is handed in by the program and passes when the run ends, and when the replies are cut off at their limit; a run cut off with nothing changed has the tests run on the commit as it was and a note; an edit that does not match its lines or is outside pool/ is refused and changes nothing" ;;
lift)
	# the toy repo grows an exam: one item whose hand lift returns 42, a header, a
	# test that is green only when the lift does, and a reader that answers from a
	# one-file "Linux" and keeps a log of what it was asked
	toy
	mkdir -p "$a/exam" "$a/lifted/PART.toy.lift@1"
	printf '[[item]]\nname  = "toy"\nasks  = "Give back the answer, the way Linux does."\nhand  = "lifted/PART.toy.lift@1/lift.c lifted/PART.toy.lift@1/lift.h"\ntests = "toy.lift@1"\n' > "$a/exam/items.toml"
	printf '#include "lift.h"\nint lift(void) { return 42; }\n' > "$a/lifted/PART.toy.lift@1/lift.c"
	printf 'int lift(void);\n' > "$a/lifted/PART.toy.lift@1/lift.h"
	cat > "$a/jig/test" <<'TOY'
#!/bin/sh
if [ "${1:-}" = toy.lift@1 ]; then
	f=lifted/PART.toy.lift@1/lift.c
	if grep -q 'return 42' $f; then echo "PASS  TST.toy.lift@1  0s  42"; echo "# 1 passed, 0 failed, 0 skipped"
	else echo "FAIL  TST.toy.lift@1  0s  the lift gives $(grep -o 'return [0-9]*' $f), not 42"; echo "# 0 passed, 1 failed, 0 skipped"; fi
	exit 0
fi
echo "FAIL  TST.toy.old@1  0s  red before any of this"; echo "# 0 passed, 1 failed, 0 skipped"
TOY
	printf '#!/bin/sh\necho "$*" >> "$HOME/reader.log"\n[ "$1" = read ] && { echo "drivers/toy.c:1: int toy_lift(void) { return 42; }"; exit 0; }\necho "reader: no such thing"; exit 1\n' > "$a/jig/reader"
	(cd "$a" && git add -A && git -c user.name=t -c user.email=t@t commit -qm exam) || fail "could not add the exam to the toy repo"
	at=$(git -C "$a" rev-parse --short=9 HEAD)
	lift() {	# lift CASE TRY: the lifter on the toy item, the replies in $t/CASE
		printf 'job = "toy"\nexam = "toy"\nat = "%s"\ntry = %s\nprompt = "builder@1"\nmodel = "qwen3.8:27b"\nnum_ctx = 8192\n' "$at" "$2" > "$t/$1.toml"
		(cd "$a" && AGENT_FAKE="$t/$1" AGENT_SSH="sh $t/ssh" AGENT_JIG=toy@jig AGENT_OUT="$t/out" AGENT_QUIET=1 \
			AGENT_RUNS="sh $here/jig/runs" RUNS="$t/runs" PRICES="$here/jig/prices.toml" AGENT_SITTING=20261030T120000Z \
			"$t/agent" build "$t/$1.toml" > "$t/$1.out" 2> "$t/$1.err"); rc=$?
	}
	mkdir "$t/whole"; for n in 1 2 3 4 5 6 7 8 9; do cp "$fx/lift/$n.json" "$t/whole/"; done
	lift whole 1; o="$t/out/toy-try1"
	[ $rc -eq 0 ] || fail "a lifter that hands in the right lift on call 9 gave exit $rc: $(head -2 "$t/whole.err" | tr '\n' ' ')$(cat "$t/whole.out")"
	s1="$t/whole/sent-1.json"
	[ "$(jq -r '[.tools[].function.name] | join(" ")' "$s1")" = "read_linux write_file submit_lift" ] ||
		fail "the lifter's tools are not read_linux, write_file and submit_lift alone: $(jq -r '[.tools[].function.name] | join(" ")' "$s1")"
	jq -r '.messages[1].content' "$s1" > "$t/first"
	grep -q '^Part to lift: Give back the answer, the way Linux does.$' "$t/first" && grep -q '^Name of the C file to write: lift.c$' "$t/first" &&
		grep -q '^The fixed header lift.h:$' "$t/first" && grep -q '^int lift(void);$' "$t/first" ||
		fail "the first message is not the question, the file to write and the header, as jig/sit gives them: $(head -c 300 "$t/first" | tr '\n' ' ')"
	grep -q 'return 42' "$t/first" "$s1" && fail "the lifter was shown the hand lift"
	ans() { jq -r "[.messages[] | select(.role == \"tool\")][$2].content" "$t/whole/sent-$1.json"; }
	ans 2 0 | grep -q '^refused: say why' || fail "a read with no why was run: $(ans 2 0)"
	ans 3 1 | grep -q '^drivers/toy.c:1: int toy_lift' || fail "a read with a why was not answered by the reader on the jig: $(ans 3 1)"
	grep -qx 'read drivers/toy.c 1 5' "$h/reader.log" && [ "$(wc -l < "$h/reader.log")" -eq 1 ] ||
		fail "the reader was not asked exactly once, for drivers/toy.c 1 5: $(cat "$h/reader.log" 2> /dev/null | tr '\n' ';')"
	ans 4 2 | grep -q "^refused: the reader has no command 'rm'" || fail "a command the reader has not was sent to the jig: $(ans 4 2)"
	ans 5 3 | grep -q '^refused: each argument is' || fail "an argument with a ; and a space was sent to the jig: $(ans 5 3)"
	ans 6 4 | grep -q '^refused: the part asks for lift.c, not other.c' || fail "a file the part does not ask for was written: $(ans 6 4)"
	[ ! -e "$o/lift/other.c" ] || fail "other.c was written"
	ans 8 6 | grep -q 'judged: fail.*FAIL  TST.toy.lift@1  0s  the lift gives return 41, not 42' || fail "a wrong lift was not shown what the test printed: $(ans 8 6)"
	rt() { sed -n "s/^$1 *= *//p" "$o/run.toml"; }
	[ "$(rt result)" = '"pass"' ] && [ "$(rt calls)" = 9 ] && [ "$(rt hand_ins)" = 2 ] && [ "$(rt exam)" = '"toy"' ] && [ "$(rt lines)" = 2 ] ||
		fail "the run is not a pass on call 9, 2 hand-ins, of exam toy, 2 lines: $(grep -E '^(result|calls|hand_ins|exam|lines)' "$o/run.toml" | tr '\n' ' ')"
	grep -qx 'tests_passed  = 1' "$o/note.toml" && grep -qx 'tests_failed  = 0' "$o/note.toml" || fail "the note is not from the item's tests: $(tr '\n' ' ' < "$o/note.toml")"
	rr=$(ls "$t"/runs/*/run.toml)
	grep -qx 'item          = "toy"' "$rr" && grep -qx 'result        = "pass"' "$rr" && grep -qx 'lines         = 2' "$rr" ||
		fail "the run in runs/ is not under the item's own name, as jig/sit records it: $(grep -E '^(item|result|lines)' "$rr" | tr '\n' ' ')"
	# written and never handed in: the program hands it in, as it does a builder's
	mkdir "$t/disk"; for n in 1 2 3; do cp "$fx/lift/disk-$n.json" "$t/disk/$n.json"; done
	lift disk 2; o="$t/out/toy-try2"
	[ $rc -eq 0 ] && [ "$(rt handed_in_by)" = '"program"' ] && [ "$(rt result)" = '"pass"' ] ||
		fail "a lift written and never handed in was not handed in by the program and passed: exit $rc, $(grep -E '^(handed_in_by|result|ended)' "$o/run.toml" | tr '\n' ' ')"
	# a hand lift that does not pass: nothing is spent
	sed -i 's/return 42/return 40/' "$a/lifted/PART.toy.lift@1/lift.c"; (cd "$a" && git -c user.name=t -c user.email=t@t commit -qam broken); at=$(git -C "$a" rev-parse --short=9 HEAD)
	mkdir "$t/broken"; cp "$fx/lift/1.json" "$t/broken/"
	lift broken 3
	[ $rc -eq 2 ] && [ ! -f "$t/broken/sent-1.json" ] && grep -q 'hand lift of toy does not pass' "$t/broken.err" ||
		fail "a hand lift that fails its own tests did not stop the lift before any call: exit $rc, $(head -1 "$t/broken.err")"
	# a part plan for an exam names its item, never tests too; its jobs carry the item
	printf 'part = "the toy lift"\nexam = "toy"\nat = "1234567"\nbudget_usd = 0\n[gear]\ntactical = 1\nbuilders = 1\n[prompts]\ndirector = "director@2"\nbuilder = "lifter@1"\n' > "$t/xplan.toml"
	"$t/agent" plan "$t/xplan.toml" > "$t/xplan.out" 2> "$t/xplan.err" && grep -q '^exam  *= "toy"$' "$t/xplan.out" && ! grep -q '^tests' "$t/xplan.out" ||
		fail "an exam plan was not read back with its item: $(cat "$t/xplan.err" "$t/xplan.out" | head -3 | tr '\n' ' ')"
	sed 's/^exam = "toy"$/exam = "toy"\ntests = ["toy.lift@1"]/' "$t/xplan.toml" > "$t/both.toml"
	"$t/agent" plan "$t/both.toml" > /dev/null 2> "$t/both.err" && fail "a plan with an exam and tests was taken"
	grep -q 'not both' "$t/both.err" || fail "a plan with an exam and tests was not refused for it: $(cat "$t/both.err")"
	cat > "$t/builder" <<'FAKE'
#!/bin/sh
jf=$2; job=$(sed -n 's/^job = "\(.*\)"$/\1/p' "$jf"); try=$(sed -n 's/^try = //p' "$jf"); cp "$jf" "$T/seen-$job.toml"
o=$AGENT_OUT/$job-try$try; mkdir -p "$o"
printf 'PASS  TST.toy.lift@1  1s  ok\n# 1 passed, 0 failed, 0 skipped\n' | "$T/agent" note "$job" "$try" 10 10 > "$o/note.toml"
printf 'result       = "pass"\nended        = "pass"\nwhy          = ""\n' > "$o/run.toml"
FAKE
	mkdir "$t/dfake"; sed 's/toy.answer@1/toy/' "$fx/direct/done/4.json" > "$t/dfake/1.json"
	printf '{"model":"qwen3.8:27b","message":{"role":"assistant","content":"","tool_calls":[{"function":{"name":"done","arguments":{"report":"Lifted."}}}]},"done":true,"done_reason":"stop","prompt_eval_count":900,"eval_count":20}\n' > "$t/dfake/2.json"
	T=$t AGENT_FAKE="$t/dfake" AGENT_BUILDER="sh $t/builder" AGENT_OUT="$t/dout" AGENT_QUIET=1 AGENT_RUNS="sh $(pwd)/jig/runs" RUNS="$t/druns" PRICES="$(pwd)/jig/prices.toml" \
		"$t/agent" direct "$t/xplan.toml" > "$t/d.out" 2> "$t/d.err"; rc=$?
	[ $rc -eq 0 ] || fail "a director on an exam plan gave exit $rc: $(cat "$t/d.err" "$t/d.out" | head -3)"
	# the director is told the part is an exam item and what its one job is: on the
	# first race run, told only "exam = serial", it asked strategic for the tests
	jq -r '.messages[1].content' "$t/dfake/sent-1.json" | grep -q 'an exam item, not a fix.*Give its one job with test "toy"' ||
		fail "the director on an exam plan is not told it is an exam item and what its one job is: $(jq -r '.messages[1].content' "$t/dfake/sent-1.json" | tail -3 | tr '\n' ' ')"
	jf=$(ls "$t"/seen-*.toml | head -1)
	grep -qx 'exam = "toy"' "$jf" && ! grep -q '^test = ' "$jf" || fail "the director's job for an exam plan does not name the item alone: $(tr '\n' ' ' < "$jf")"
	echo "a lifter on a toy exam item: offered read_linux, write_file and submit_lift alone; shown the question, the file to write and the header, as jig/sit shows them, and never the hand lift; a read with no why refused, one with a why asked of the reader on the jig, once; a command the reader has not, or an argument with a ; or a space, never sent; a file the part does not ask for refused; a wrong lift shown what the test printed; the right one a pass on call 9, recorded under the item's own name with its lines, the note from the item's tests; a lift written and never handed in handed in by the program; a hand lift that fails its tests stops it before any call; an exam plan names its item, never tests too, and its director's jobs carry the item" ;;
columns)
	# a builder's note says its calls, its peak context and its wall minutes, and its run
	# in runs/ its calls and seconds
	toy
	mkdir "$t/whole"; cp "$fx"/build/[0-9].json "$t/whole/"
	run whole; o="$t/out/toy-try1"
	[ $rc -eq 0 ] || fail "the builder on the toy gave exit $rc: $(head -2 "$t/whole.err" | tr '\n' ' ')"
	grep -qx 'calls         = 8' "$o/note.toml" && grep -qx 'peak_context  = 2250' "$o/note.toml" && grep -qE '^minutes       = [0-9]+$' "$o/note.toml" ||
		fail "the builder's note does not say its 8 calls, its peak context of 2250 and its minutes: $(tr '\n' ' ' < "$o/note.toml" | cut -c1-300)"
	rr=$(ls "$t"/runs/*/run.toml)
	grep -qx 'calls         = 8' "$rr" && grep -qE '^seconds       = [0-9]+$' "$rr" || fail "the builder's run in runs/ does not say its calls and seconds: $(tail -4 "$rr" | tr '\n' ' ')"
	# a director adds its own peak context, as of the job, to each note it is shown, and
	# its run in runs/ says its calls and seconds
	cat > "$t/builder" <<'FAKE'
#!/bin/sh
jf=$2; job=$(sed -n 's/^job = "\(.*\)"$/\1/p' "$jf"); try=$(sed -n 's/^try = //p' "$jf")
o=$AGENT_OUT/$job-try$try; mkdir -p "$o"
printf 'PASS  TST.share.waits@1  1s  ok\n# 1 passed, 0 failed, 0 skipped\n' | "$T/agent" note "$job" "$try" 10 10 > "$o/note.toml"
printf 'result       = "pass"\nended        = "pass"\nwhy          = ""\n' > "$o/run.toml"
FAKE
	mkdir "$t/fake"; sed 's/toy.answer@1/share.waits@1/' "$fx/direct/done/4.json" > "$t/fake/1.json"
	printf '{"model":"qwen3.8:27b","message":{"role":"assistant","content":"","tool_calls":[{"function":{"name":"done","arguments":{"report":"Passed."}}}]},"done":true,"done_reason":"stop","prompt_eval_count":900,"eval_count":20}\n' > "$t/fake/2.json"
	T=$t AGENT_FAKE="$t/fake" AGENT_BUILDER="sh $t/builder" AGENT_OUT="$t/dout" AGENT_QUIET=1 AGENT_RUNS="sh $(pwd)/jig/runs" RUNS="$t/druns" PRICES="$(pwd)/jig/prices.toml" \
		"$t/agent" direct agent/plans/waits.toml > "$t/d.out" 2> "$t/d.err"; rc=$?
	[ $rc -eq 0 ] || fail "a director on waits gave exit $rc: $(cat "$t/d.err" "$t/d.out" | head -3)"
	grep -qx 'director_peak = 1200' "$t"/dout/waits/jobs/waits-1-try1/note.toml || fail "the note does not say the director's peak context as of the job, 1200: $(tail -3 "$t"/dout/waits/jobs/waits-1-try1/note.toml | tr '\n' ' ')"
	jq -r '[.messages[] | select(.role == "tool")][0].content' "$t/fake/sent-2.json" | grep -qx 'director_peak = 1200' || fail "the director was not shown its own peak in the note"
	dr=$(ls "$t"/druns/*/run.toml)
	grep -qx 'calls         = 2' "$dr" && grep -qE '^seconds       = [0-9]+$' "$dr" || fail "the director's run in runs/ does not say its 2 calls and its seconds: $(tail -4 "$dr" | tr '\n' ' ')"
	# the card: each part with its builders' calls and biggest peak, the director's peak, and
	# its wall minutes; a part from before these columns shows a dash, not a guess
	rec() {	# rec ITEM TRY CALLS SECONDS PEAK SITTING
		RUNS_CALLS=$3 RUNS_SECONDS=$4 RUNS_PEAK=$5 RUNS_SITTING=$6 RUNS_BENCH=agent RUNS_ENDED=pass RUNS="$t/card-runs" PRICES="$(pwd)/jig/prices.toml" \
			sh jig/runs record "$1" ollama:qwen3.8:27b 2026-10-29 "$2" 100 10 pass 0 "$t/d.out" "$t/d.out" > /dev/null || fail "jig/runs would not record $1 with its calls and seconds"
	}
	rec fix-share-waits 1 40 2515 74727 20261029T160914Z; rec fix-share-waits 2 41 1900 60593 20261029T160914Z
	rec fix-share-waits 3 27 1667 43648 20261029T160914Z; rec direct-waits 1 7 6780 2976 20261029T160914Z
	RUNS_BENCH=agent RUNS_ENDED=pass RUNS_PEAK=900 RUNS_SITTING=20261028T100000Z RUNS="$t/card-runs" PRICES="$(pwd)/jig/prices.toml" \
		sh jig/runs record direct-older ollama:qwen3.8:27b 2026-10-28 1 100 10 pass 0 "$t/d.out" "$t/d.out" > /dev/null || fail "an older director run could not be recorded"
	RUNS="$t/card-runs" sh jig/scorecard > "$t/card" 2>&1 || fail "the scorecard failed: $(head -2 "$t/card")"
	sed -n '/^The layered bench, by part$/,$p' "$t/card" > "$t/parts"
	grep -q '^  waits .*3 builders, 108 calls, peak 74727  director peak 2976  113 minutes' "$t/parts" ||
		fail "the card does not show waits with 3 builders, 108 calls, peak 74727, director peak 2976 and 113 minutes: $(cat "$t/parts")"
	grep -q '^  older .*0 builders, - calls, peak -  director peak 900  - minutes' "$t/parts" ||
		fail "a part recorded before the columns is not shown with dashes: $(grep older "$t/parts")"
	echo "a builder's note says its calls, peak context and minutes, and its run its calls and seconds; a director adds its own peak, as of the job, to each note it is shown, and records its calls and seconds; the card shows each part's builders, their calls and biggest peak, the director's peak and the wall minutes, and dashes for a part from before" ;;
direct)
	# the director's replies are saved ones; its builders are a stand-in that hands back,
	# for each try, the result the case's script line says, as a real note (agent note)
	cat > "$t/builder" <<'FAKE'
#!/bin/sh
# fake builder: build JOBFILE; the line of $T/script numbered by the job's try: pass, or fail and the failing tests
jf=$2; job=$(sed -n 's/^job = "\(.*\)"$/\1/p' "$jf"); try=$(sed -n 's/^try = //p' "$jf")
cp "$jf" "$T/seen-$job.toml"; n=$(sed -n 's/^hint = "\(.*\)"$/\1/p' "$jf"); [ -z "$n" ] || cp "$n" "$T/seen-$job.hint"
set -- $(sed -n "${try}p" "$T/script"); res=$1; shift
o=$AGENT_OUT/$job-try$try; mkdir -p "$o"
{ for f in "$@"; do echo "FAIL  TST.$f  1s  $f is still red, and the builder's fix did not change what the probe saw on the screen at all"; done
  echo "PASS  TST.toy.base@1  0s  fine"; echo "# summary"; } | "$T/agent" note "$job" "$try" 1000 60 pool/PART.toy@1/toy.c > "$o/note.toml"
printf 'result       = "%s"\nended        = "%s"\nwhy          = ""\ndollars      = %s\n' "$res" "$res" "${DOLLARS:-0}" > "$o/run.toml"
[ "$res" = pass ]
FAKE
	plan() {	# plan CASE [EXTRA-LINES]: two toy tests, gear 1, the library's prompts
		printf 'part = "PART.toy@1: the answer"\ntests = ["toy.answer@1", "toy.other@1"]\nat = "1234567"\nbudget_usd = %s\n%s\n[gear]\ntactical = %s\nbuilders = %s\n\n[prompts]\ndirector = "director@1"\nbuilder = "builder@1"\n' "${USD:-0}" "${2:-}" "${GT:-1}" "${GB:-1}" > "$t/$1.toml"
	}
	direct() {	# direct CASE SCRIPT [EXTRA-LINES]: $rc, $t/out/CASE/
		mkdir -p "$t/$1"; cp "$fx/direct/$1"/*.json "$t/$1/" 2> /dev/null; printf '%s\n' "$2" | tr '|' '\n' > "$t/script"
		plan "$1" "${3:-}"
		T=$t AGENT_FAKE="$t/$1" AGENT_BUILDER="sh $t/builder" AGENT_OUT="$t/out" AGENT_QUIET=1 AGENT_CARD_CTX=${CTX:-98304} \
			AGENT_RUNS="sh $(pwd)/jig/runs" RUNS="$t/runs-$1" PRICES="$(pwd)/jig/prices.toml" \
			"$t/agent" direct "$t/$1.toml" > "$t/$1.out" 2> "$t/$1.err"; rc=$?
		o="$t/out/$1"
	}
	tool() { jq -r "[.messages[] | select(.role == \"tool\")][$2].content" "$t/$1/sent-$3.json"; }
	direct done 'fail toy.answer@1 toy.other@1|pass|pass'
	[ $rc -eq 0 ] || fail "a director whose tests both passed gave exit $rc: $(cat "$t/done.out" "$t/done.err" | head -3 | tr '\n' ' ')"
	tool done 0 2 | grep -q '^builder@1:[0-9]*: - An instruction the processor may not have.*CPUID' || fail "search_prompts did not find CPUID in the library's builder@1: $(tool done 0 2 | head -1)"
	tool done 1 3 | grep -q '^refused: say why' || fail "a job with no why started a builder: $(tool done 1 3)"
	tool done 2 4 | grep -q '^refused: nosuch@1 is not a test in the plan.  The plan.s tests: toy.answer@1 toy.other@1$' || fail "a test not in the plan was given out: $(tool done 2 4)"
	[ "$(ls "$t"/seen-done-*.toml | wc -l)" -eq 3 ] || fail "$(ls "$t"/seen-done-*.toml | wc -l) builders started, not 3"
	j1="$t/seen-done-1.toml"
	for want in 'job = "done-1"' 'test = "toy.answer@1"' 'at = "1234567"' 'try = 1' 'prompt = "builder@1"' 'model = "qwen3.8:27b"' 'num_ctx = 98304'; do
		grep -qxF "$want" "$j1" || fail "the first job does not say $want: $(tr '\n' ' ' < "$j1")"
	done
	[ "$(cat "$t/seen-done-1.hint")" = "The answer is set in toy.c, the only file of the part." ] || fail "the director's hint did not reach the builder"
	tool done 3 5 | grep -q '^Job done-1, toy.answer@1: fail, ended fail.$' && tool done 3 5 | grep -q '^failing       = \["TST.toy.answer@1", "TST.toy.other@1"\]$' ||
		fail "the director was not shown the program's note for the job: $(tool done 3 5 | head -4 | tr '\n' ' ')"
	tool done 5 7 | grep -q '^refused: not every test in the plan has passed.  Not yet: toy.other@1$' || fail "done was taken with toy.other@1 never passed: $(tool done 5 7)"
	grep -q '^# done: done$' "$o/report.md" && grep -q 'in 3 job(s)' "$o/report.md" && grep -q 'the hint fixed it' "$o/report.md" && [ ! -f "$o/question.md" ] ||
		fail "the part report does not say done in 3 jobs with the director's words, or a question was left: $(head -3 "$o/report.md" | tr '\n' ' ')"
	dr=$(ls "$t"/runs-done/*/run.toml 2> /dev/null)
	[ "$(echo "$dr" | grep -c .)" -eq 1 ] && grep -qx 'item          = "direct-done"' "$dr" && grep -qx 'result        = "pass"' "$dr" &&
		grep -qx 'input_tokens  = 12400' "$dr" && grep -qx 'ended         = "pass"' "$dr" && grep -qx 'bench         = "agent"' "$dr" ||
		fail "the director's own run is not recorded in runs/ with its tokens and ending: $(grep -E '^(item|result|input_tokens|ended)' $dr | tr '\n' ' ')"
	grep -q '^hint: Change only the number' "$o/log.md" && grep -q '^result: pass, ended pass$' "$o/log.md" || fail "the log does not keep the hints and the results"
	# stuck: the same test failing three runs in a row stops the part; no fourth call
	direct stuck 'fail toy.answer@1|fail toy.answer@1|fail toy.answer@1|pass'
	[ $rc -eq 1 ] && [ "$(cat "$t/stuck/calls")" = 3 ] && grep -q '^# stuck stopped: stuck$' "$o/question.md" &&
		grep -qx 'ended         = "stuck"' "$t"/runs-stuck/*/run.toml &&
		grep -q 'the same 1 test(s) failed in the last 3 runs in a row on toy.answer@1: TST.toy.answer@1' "$o/question.md" ||
		fail "three runs with the same failure did not stop the part as stuck: exit $rc, $(cat "$t/stuck/calls") calls, $(head -3 "$o/question.md" 2> /dev/null | tr '\n' ' ')"
	[ "$(grep -c '^stuck-[0-9]:$' "$o/question.md")" -eq 3 ] || fail "the question note does not hold the last three notes"
	# the same failures on jobs that move between tests are not stuck: each starts from the plan's commit
	direct rotate 'fail toy.answer@1 toy.other@1|fail toy.answer@1 toy.other@1|fail toy.answer@1 toy.other@1'
	[ $rc -eq 1 ] && grep -q '^# rotate stopped: asked$' "$o/question.md" ||
		fail "three jobs on two tests, failing the same way, were called stuck: $(head -1 "$o/question.md")"
	# no drop: two failing, then three, then two: not below the first low in the last two runs
	direct nodrop 'fail toy.answer@1 toy.other@1|fail toy.answer@1 toy.other@1 toy.base@1|fail toy.answer@1 toy.base@1|pass' 'stuck_no_drop = 2'
	[ $rc -eq 1 ] && grep -q 'the failing count has not gone below 2 in the last 2 runs' "$o/question.md" ||
		fail "a failing count that did not drop did not stop the part: exit $rc, $(sed -n 3p "$o/question.md" 2> /dev/null)"
	# out of tries: the plan's two tries used, the third job refused, no builder started
	direct tries 'fail toy.answer@1|fail toy.other@1|pass' 'max_tries = 2'
	[ $rc -eq 1 ] && grep -q 'all 2 tries in the plan were used' "$o/question.md" && [ ! -f "$t/seen-tries-3.toml" ] ||
		fail "a third job past the plan's two tries was run, or the part did not stop: exit $rc"
	# ask: the part waits, the question is in the note for strategic
	direct ask ''
	[ $rc -eq 1 ] && grep -q '^# ask stopped: asked$' "$o/question.md" && grep -q 'but the spec says 41.  Which is right?' "$o/question.md" ||
		fail "a question from the director did not stop the part with it in the note: exit $rc"
	# fold: on a 2048-token card the notes fill the director's room; its older turns are
	# folded into the part's standing, which names every job, and the last four are kept
	many() { awk -v k="$1" 'BEGIN { printf "fail toy.answer@1"; for (i = 1; i <= 18; i++) printf " toy.f%d-%02d@1", k, i }'; }
	CTX=2048 direct fold "$(many 1)|$(many 2)|$(many 3)|$(many 4)|$(many 5)|pass"
	f6=$(jq -r '.messages[2].content' "$t/fold/sent-6.json")
	printf '%s' "$f6" | grep -q '^Where the part stands' || fail "the director's older turns were not folded on a full card: $(printf '%s' "$f6" | head -c 100)"
	for k in 1 2 3 4 5; do printf '%s' "$f6" | grep -q "^- fold-$k, toy.answer@1: fail, ended fail$" || fail "the fold lost job fold-$k"; done
	printf '%s' "$f6" | grep -q 'toy.other@1 (not yet)' || fail "the fold lost a test in the plan"
	[ "$(printf '%s' "$f6" | grep -c '^job           = "fold-[345]"$')" -eq 3 ] && ! printf '%s' "$f6" | grep -q '^job           = "fold-[12]"$' ||
		fail "the fold does not keep the last three notes whole, and only those"
	jq -e '.messages[1].content | startswith("The part plan, fold:")' "$t/fold/sent-6.json" > /dev/null || fail "the plan was folded away"
	[ "$(jq '.messages | length' "$t/fold/sent-6.json")" -eq 7 ] || fail "the fold did not keep exactly the system prompt, the plan, the standing and the last four messages"
	# gear 2: a paid layer with no dollars does not start; with a few, it stops before a call it cannot pay for
	GB=2 direct broke 'pass'
	[ $rc -eq 2 ] && grep -q "budget_usd is 0" "$t/broke.err" || fail "a plan with paid builders and no dollars gave exit $rc: $(head -1 "$t/broke.err")"
	cp "$fx/bedrock/aws" "$t/aws"; chmod 600 "$t/aws"
	AIOS_AWS="$t/aws" AGENT_MAX_COST=0.01 DOLLARS=0.006 USD=0.02 GT=2 GB=2 direct paid 'fail toy.answer@1|pass'
	[ $rc -eq 1 ] && [ ! -f "$t/paid/sent-2.json" ] && grep -q '^# paid stopped: budget$' "$o/question.md" ||
		fail "a director with \$0.0095 left and a call that may cost \$0.01 made it, or did not stop on the budget: exit $rc, $(cat "$t/paid/calls") calls"
	for want in 'model = "global.anthropic.claude-sonnet-4-6"' 'num_ctx = 200000' 'gear = 2' 'max_usd = 0.015500'; do
		grep -qxF "$want" "$t/seen-paid-1.toml" || fail "the paid builder's job does not say $want: $(tr '\n' ' ' < "$t/seen-paid-1.toml")"
	done
	grep -qx 'cost_usd      = 0.004500' "$t"/runs-paid/*-direct-paid-*/run.toml && grep -qx 'ended         = "budget"' "$t"/runs-paid/*-direct-paid-*/run.toml ||
		fail "the paid director's run is not in runs/ with its cost and ending"
	echo "a director: search_prompts finds CPUID in builder@1, a job with no why or a test not in the plan starts no builder, each job written with the plan's commit, prompt and the hint, the program's note shown after each, done refused until both tests passed, then done in 3 jobs with its report and log; stuck after the same failure three runs in a row on one test but not across two, with no fourth call and the last three notes for strategic; stuck when the count did not drop; a third try past a plan of two refused; a question stops the part; on a full card older turns folded into a standing that names every job and test, the last three notes whole, the plan and the last four messages kept; on gear 2, no dollars is no start, a paid builder's job carries Sonnet, its context and what is left of the part's share, and the director stops before a call it cannot pay for, recorded with its cost" ;;
bedrock)
	command -v jq > /dev/null || fail "no jq here to read what the agent sent"
	# a key file that is not a key, readable only by its owner, as jig/model wants it
	cp "$fx/bedrock/aws" "$t/aws"; chmod 600 "$t/aws"
	call() {	# call REPLY: one gear-2 chat with REPLY as Bedrock's answer
		mkdir "$t/$1"; [ -f "$fx/bedrock/$1.json" ] && cp "$fx/bedrock/$1.json" "$t/$1/1.json"
		AIOS_AWS="$t/aws" AGENT_GEAR=2 AGENT_FAKE="$t/$1" "$t/agent" chat global.anthropic.claude-sonnet-4-6 200000 2000 \
			< "$fx/chat/conversation.json" > "$t/$1/out" 2> "$t/$1/err"; rc=$?
	}
	field() { sed -n "s/^$2 *= //p" "$t/$1/out"; }
	call tool
	[ $rc -eq 0 ] || fail "a Bedrock reply asking for a tool gave exit $rc: $(head -1 "$t/tool/err")"
	[ "$(cat "$t/tool/sent-1.url")" = "https://bedrock-runtime.us-west-2.amazonaws.com/model/global.anthropic.claude-sonnet-4-6/converse aws:amz:us-west-2:bedrock" ] ||
		fail "the call did not go to Converse in the key file's region, signed for bedrock there: $(cat "$t/tool/sent-1.url")"
	s="$t/tool/sent-1.json"
	[ "$(jq -r '.system[0].text' "$s")" = "You are a builder.  One job, then submit." ] && [ "$(jq '.inferenceConfig.maxTokens' "$s")" = 2000 ] ||
		fail "the system prompt or the reply's limit is not where Converse wants them: $(jq -c '{system, inferenceConfig}' "$s")"
	[ "$(jq -r '[.messages[].role] | join(" ")' "$s")" = "user assistant user" ] || fail "the turns do not alternate: $(jq -c '[.messages[].role]' "$s")"
	id=$(jq -r '.messages[1].content[] | select(.toolUse) | .toolUse.toolUseId' "$s")
	[ -n "$id" ] && [ "$(jq -r '.messages[2].content[0].toolResult.toolUseId' "$s")" = "$id" ] &&
		[ "$(jq -r '.messages[2].content[0].toolResult.content[0].text' "$s")" = "/* loader.c */" ] ||
		fail "the tool's answer is not handed back as the result of the call it answers: $(jq -c '.messages[1:]' "$s")"
	[ "$(jq -r '.messages[2].content[1].text' "$s")" = "Call 1 of 60 done; 59 left." ] ||
		fail "a note after a tool's answer is not joined to the same user turn: $(jq -c '.messages[2]' "$s")"
	[ "$(jq -c '.messages[1].content[] | select(.toolUse) | .toolUse.input' "$s")" = "$(jq -c '.messages[1].tool_calls[0].function.arguments' "$fx/chat/conversation.json")" ] ||
		fail "the call's arguments are not handed over as its input"
	[ "$(jq -r '.toolConfig.tools[0].toolSpec | .name + " " + (.inputSchema.json.required | join(","))' "$s")" = "look command,why" ] ||
		fail "the tools are not offered as Converse's toolSpecs: $(jq -c .toolConfig "$s")"
	[ "$(field tool stop)" = '"tool"' ] && [ "$(field tool tokens_in)" = 1000 ] && [ "$(field tool tokens_out)" = 100 ] ||
		fail "a Bedrock tool call is not read as one, with its tokens: $(head -3 "$t/tool/out" | tr '\n' ' ')"
	[ "$(field tool message | jq -r '.tool_calls[0].function.arguments.why')" = "see where the flag is set" ] ||
		fail "the tool call is not handed back in the conversation's own shape, with its why"
	call end; [ "$(field end stop)" = '"end"' ] || fail "end_turn is read as $(field end stop)"
	call cut; [ "$(field cut stop)" = '"max_tokens"' ] || fail "max_tokens is read as $(field cut stop)"
	call full; [ "$(field full stop)" = '"context"' ] || fail "a full context is read as $(field full stop)"
	# a key anyone can read is not used, and nothing is sent
	chmod 644 "$t/aws"; call open
	[ $rc -eq 1 ] && grep -q 'chmod 600' "$t/open/err" && [ ! -f "$t/open/sent-1.json" ] ||
		fail "a key file others can read gave exit $rc, said $(head -1 "$t/open/err"), and sent $(ls "$t/open" | grep -c sent)"
	echo "gear 2: one call to Converse in the key file's region, signed for bedrock; the system prompt, the reply's limit, turns that alternate, each tool answer handed back as the result of its own call and a note after it joined to the same turn, the tools as toolSpecs; a tool call read back with its why and tokens; end_turn, max_tokens and a full context told apart; a key others can read refused before anything is sent" ;;
*)	fail "tests/programs/agent.sh needs a mode: runtime, note, plan, chat, build, limit, direct or bedrock" ;;
esac
