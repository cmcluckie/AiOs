#!/bin/sh
# Probe for TST.draft.parses@1: jig/draft with saved replies standing in for the
# model (MODEL_FAKE), so it costs nothing and calls nothing.  A saved reply that
# is a proper petition (after the model has asked the reader a question) must be
# kept, byte for byte, the same both times; a reply that is prose, one the
# referee refuses and an empty one must each be thrown away: exit 1, the reason
# printed, no draft file anywhere, and the call still written in the run record.
# Mode source (TST.draft.source@1): told DRAFT_SOURCE=bsd, every question the
# reader gets names bsd, the first message names Berkeley and not V7 and carries
# the petition to supersede, the default is v7, and an unknown source makes no call.
# Exit 0 is green, 1 is red, 2 means this host cannot run it (no jq).
set -u
cd "$(dirname "$0")/../.."
command -v jq > /dev/null || { echo "jq is not installed"; exit 2; }
fx=tests/fixtures/draft
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail() { echo "$*"; exit 1; }
export READER="sh $PWD/$fx/reader-stub.sh" STUBLOG="$tmp/asked.log"
export DRAFT_ID=0012 DRAFT_TODAY=2026-11-01 DRAFT_EXPIRES=2027-05-01 DRAFT_TRIES=1

# attempt NAME OUTDIR: one draft from the saved replies in $fx/NAME, its records in $tmp/runs-NAME
attempt() {
	rm -rf "$tmp/replies"; cp -r "$fx/$1" "$tmp/replies"
	RUNS="$tmp/runs-$1" MODEL_FAKE="$tmp/replies" DRAFT_OUT="$2" sh jig/draft PUR.0011@1 usr/src/cmd/cat.c > "$tmp/out-$1" 2> "$tmp/err-$1"
}
ran() { ls "$tmp/runs-$1" 2> /dev/null | wc -l; }
result_of() { cat "$tmp/runs-$1"/*/run.toml 2> /dev/null | sed -n 's/^result *= *"\(.*\)".*/\1/p'; }

if [ "${1:-}" = source ]; then
	attempt good "$tmp/o0" || fail "the default source: a proper petition was not kept"
	grep -v -- "^--source v7 " "$tmp/asked.log" | grep -q . && fail "with no source given, a question reached the reader without --source v7: $(grep -v -- '^--source v7 ' "$tmp/asked.log" | head -1)"
	: > "$tmp/asked.log"; rm -rf "$tmp/runs-good"
	DRAFT_SOURCE=bsd DRAFT_SUPERSEDES=0012 attempt good "$tmp/o1" || fail "told bsd, a proper petition was not kept: $(tail -1 "$tmp/err-good")"
	[ -s "$tmp/asked.log" ] || fail "told bsd, the reader was never asked anything"
	grep -v -- "^--source bsd " "$tmp/asked.log" | grep -q . && fail "told bsd, a question reached the reader as '$(grep -v -- '^--source bsd ' "$tmp/asked.log" | head -1)'"
	first=$(jq -r '.[0].content[0].text' "$tmp/runs-good"/*/input.txt)
	printf '%s' "$first" | grep -q "4.4BSD-Lite" || fail "told bsd, the model's first message does not say 4.4BSD-Lite"
	printf '%s' "$first" | grep -q "Unix Version 7" && fail "told bsd, the model's first message still says Unix Version 7"
	printf '%s' "$first" | grep -q 'supersedes = "0012"' || fail "the petition to supersede is not in the model's first message"
	rm -rf "$tmp/runs-plan9"; cp -r "$fx/good" "$tmp/replies"
	DRAFT_SOURCE=plan9 RUNS="$tmp/runs-plan9" MODEL_FAKE="$tmp/replies" DRAFT_OUT="$tmp/o9" sh jig/draft PUR.0011@1 usr/src/cmd/cat.c > /dev/null 2> "$tmp/err-plan9"; rc=$?
	[ $rc -eq 1 ] || fail "an unknown source gave exit status $rc, not a refusal"
	grep -q "unknown source" "$tmp/err-plan9" || fail "an unknown source was refused, but the reason says '$(tail -1 "$tmp/err-plan9")'"
	[ "$(ran plan9)" -eq 0 ] || fail "an unknown source still made a call"
	echo "every question named its source (v7 by default, bsd when told), the first message named 4.4BSD-Lite and not V7 and carried the petition to supersede, and an unknown source was refused before any call"
	exit 0
fi

attempt good "$tmp/o1"; rc=$?
[ $rc -eq 0 ] || fail "a proper petition was not kept (exit status $rc): $(tail -1 "$tmp/err-good")"
cmp -s "$tmp/o1/0012/petition.toml" "$fx/petition-good.toml" || fail "the kept draft is not the model's reply, byte for byte"
grep -q "usr/src/cmd/cat.c 1 10" "$tmp/asked.log" || fail "the model asked the reader to read cat.c and the reader was not asked"
[ "$(ran good)" -eq 1 ] && [ "$(result_of good)" = pass ] || fail "the kept draft is not one run recorded as pass ($(ran good) runs, '$(result_of good)')"
rm -rf "$tmp/runs-good"
attempt good "$tmp/o2" || fail "the same reply, the second time, was not kept"
cmp -s "$tmp/o1/0012/petition.toml" "$tmp/o2/0012/petition.toml" || fail "the same reply gave two different drafts"

throw() {	# NAME FRAGMENT
	attempt "$1" "$tmp/o-$1"; rc=$?
	[ $rc -eq 1 ] || fail "$1: exit status $rc, not a refusal"
	grep -q -- "$2" "$tmp/err-$1" || fail "$1: refused, but the reason says '$(tail -1 "$tmp/err-$1")', not '$2'"
	[ "$(find "$tmp/o-$1" -type f 2> /dev/null | wc -l)" -eq 0 ] || fail "$1: thrown away, but a file was left behind"
	[ ! -s "$tmp/out-$1" ] || fail "$1: thrown away, but it said it kept something"
	[ "$(ran "$1")" -eq 1 ] && [ "$(result_of "$1")" = refused ] || fail "$1: the paid-for call is not one run recorded as refused ($(ran "$1") runs, '$(result_of "$1")')"
}
throw prose "no \[petition\] table"
throw preface "is not TOML"
throw refused "the role 'chief'"
throw empty "reply was empty"
echo "a petition read, kept and the same both times; prose, prose in front of a petition, a refused petition and an empty reply each thrown away with nothing left behind and the call recorded"
