#!/bin/sh
# Probe for TST.referee.draft@1: `referee petition FILE` on a draft petition.  A
# good fixture (tests/fixtures/draft/petition-good.toml, written by hand as a
# stand-in for a model's draft) must be accepted; then it is spoiled nine ways,
# one at a time, and every spoiled copy must be refused with the reason named.
# Exit 0 is green, 1 is red.
set -u
cd "$(dirname "$0")/../.."
good=tests/fixtures/draft/petition-good.toml
ref="sh pool/AGT.referee@1/referee petition"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail() { echo "$*"; exit 1; }

$ref "$good" > "$tmp/out" 2> "$tmp/err" || fail "the good draft was refused: $(head -1 "$tmp/err")"
grep -q "well-formed, 2 component(s) pending admission" "$tmp/out" || fail "the good draft was accepted, but the referee said '$(head -1 "$tmp/out")'"
grep -qx "pending PART.cat@1" "$tmp/out" || fail "the good draft's unadmitted component is not listed"

# spoil WHAT FRAGMENT SED: a copy of the good draft with one thing wrong must be refused (exit 1), naming FRAGMENT
spoil() {
	what=$1; frag=$2; shift 2
	sed "$@" "$good" > "$tmp/bad.toml"
	cmp -s "$good" "$tmp/bad.toml" && fail "$what: the spoiling changed nothing"
	$ref "$tmp/bad.toml" > "$tmp/out" 2> "$tmp/err"; rc=$?
	[ $rc -eq 1 ] || fail "$what: accepted or crashed (exit status $rc)"
	grep -q -- "$frag" "$tmp/err" || fail "$what: refused, but the reason says '$(head -1 "$tmp/err")', not '$frag'"
	[ ! -s "$tmp/out" ] || fail "$what: refused, but still printed an answer"
}
printf 'Sure, here is a petition for cat.\n' > "$tmp/prose.toml"
$ref "$tmp/prose.toml" > "$tmp/out" 2> "$tmp/err"; rc=$?
[ $rc -eq 1 ] && grep -q "no \[petition\] table" "$tmp/err" || fail "prose: exit status $rc and '$(head -1 "$tmp/err")'"
spoil "a role the schema does not have" "the role 'chief'" -e 's/^role     = "primary"/role     = "chief"/'
spoil "a petition that has expired" "rule 9" -e 's/^expires    = .*/expires    = 2020-01-01/'
spoil "a number with no version" "PART.cat has no version\|PART.cat is not\|has no version" -e 's/^code     = "PART.cat@1"/code     = "PART.cat"/'
spoil "a purpose that does not exist" "rule 1: missing PUR.9999@1" -e 's/PUR.0011@1/PUR.9999@1/'
spoil "a reason that is not a SPEC code" "rule 2" -e '0,/^reason *= *".*"/s//reason = "because it prints"/'
spoil "a component with no admitted flag" "no admitted" -e '0,/^admitted = false/{/^admitted = false/d}'
spoil "a table that is not one" "is not a catalogue number" -e 's/^code     = "PART.loader@9"/code     = "THING.loader@9"/'
printf '\n[[component]]\ncode     = "PART.nothing-here@1"\nrole     = "dependency"\nadmitted = true\n' | cat "$good" - > "$tmp/bad.toml"
$ref "$tmp/bad.toml" > "$tmp/out" 2> "$tmp/err"; rc=$?
[ $rc -eq 1 ] && grep -q "rule 1: missing PART.nothing-here@1" "$tmp/err" || fail "a number that resolves nowhere, with no component to bring it: exit status $rc and '$(head -1 "$tmp/err")'"
echo "a good draft accepted and listed; prose and eight spoiled copies refused, each naming its reason"
