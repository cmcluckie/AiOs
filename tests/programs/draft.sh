#!/bin/sh
# Probe for TST.draft.parses@1: jig/draft with saved replies standing in for the
# model (MODEL_FAKE), so it costs nothing and calls nothing.  A saved reply that
# is a proper petition (after the model has asked the reader a question) must be
# kept, byte for byte, the same both times; a reply that is prose, one the
# referee refuses and an empty one must each be thrown away: exit 1, the reason
# printed, no draft file anywhere, and the call still written in the run record.
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
throw refused "the role 'chief'"
throw empty "reply was empty"
echo "a petition read, kept and the same both times; prose, a refused petition and an empty reply each thrown away with nothing left behind and the call recorded"
