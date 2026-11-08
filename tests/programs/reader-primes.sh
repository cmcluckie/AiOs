#!/bin/sh
# Probe for TST.reader.primes-pin@1: jig/reader asked about Dave Plummer's drag race, against a
# real clone of PlummersSoftwareLLC/Primes at the commit in jig/primes.pin.  Checks that the
# lines it hands back are the drag race's lines byte for byte, taken a different way from
# the reader (out of an archive); that it answers from the pin and nothing else,
# so a Linux or Berkeley path is refused and does not fall back to either; and that find,
# callers and calls, which need Linux's cscope index, say they are Linux-only.
# Exit 0 is green, 1 is red, 2 means this host cannot run it (no clone, no pin).
set -u
cd "$(dirname "$0")/../.."
PRIMES=${PRIMES:-$PWD/../Primes}; export PRIMES
[ -d "$PRIMES/.git" ] || { echo "no clone of Primes at $PRIMES (set PRIMES=)"; exit 2; }
pin=$(sed -n 's/^commit *= *"\([0-9a-f]*\)".*/\1/p' jig/primes.pin)
[ -n "$pin" ] || { echo "no commit in jig/primes.pin"; exit 2; }
git -C "$PRIMES" cat-file -e "$pin^{commit}" 2> /dev/null || { echo "the clone at $PRIMES lacks the pinned commit $pin"; exit 2; }
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
R="sh jig/reader --source primes"
F=PrimeC/solution_1/sieve.c
fail() { echo "$*"; exit 1; }

# refuse WHAT FRAGMENT COMMAND...: exit status 1 and FRAGMENT on stderr, nothing on stdout
refuse() {
	what=$1; frag=$2; shift 2
	"$@" > "$tmp/out" 2> "$tmp/err"; rc=$?
	[ $rc -eq 1 ] || fail "$what: exit status $rc, not a refusal"
	grep -q -- "$frag" "$tmp/err" || fail "$what: refused, but the reason says '$(head -1 "$tmp/err")', not '$frag'"
	[ ! -s "$tmp/out" ] || fail "$what: refused, but still printed something"
}

# the pin is the drag race's pin, not Linux's or Berkeley's
[ "$($R pin)" = "$pin" ] || fail "pin says '$($R pin)', not the commit in jig/primes.pin"

# the lines are the drag race's lines, byte for byte, taken a different way: out of an archive
git -C "$PRIMES" archive "$pin" "$F" | tar -xO > "$tmp/want"
# sieve.c has no newline after its last line; the reader ends every line with one
[ -z "$(tail -c1 "$tmp/want")" ] || echo >> "$tmp/want"
total=$(awk 'END { print NR }' "$tmp/want")
[ "$total" -gt 10 ] || fail "the archived $F has only $total lines"
# at most 200 lines come back at a time, so the file is read in two goes
{ $R read "$F" 1 200 && $R read "$F" 201 "$total"; } > "$tmp/got" || fail "read refused lines that are there"
sed "s|^$F:[0-9]*: ||" "$tmp/got" > "$tmp/text"
cmp -s "$tmp/text" "$tmp/want" || fail "the lines read from $F differ from the archive of the pin"
awk -v f="$F" '{ if ($0 !~ "^" f ":" NR ": ") bad = 1 } END { exit bad }' "$tmp/got" || fail "the lines are not numbered 1 to $total"
$R read "$F" 5 9 > "$tmp/got"
[ "$(sed -n 5p "$tmp/want")" = "$(sed -n "1s|^$F:5: ||p" "$tmp/got")" ] || fail "a range from the middle starts at the wrong line"
[ "$(wc -l < "$tmp/got")" -eq 5 ] || fail "a range of 5 lines came back as $(wc -l < "$tmp/got")"

# list is the pinned tree
$R list PrimeC/solution_1 > "$tmp/out" || fail "list refused a directory that is there"
git -C "$PRIMES" ls-tree --name-only "$pin:PrimeC/solution_1" | grep -x sieve.c > /dev/null || fail "the archive has no sieve.c in PrimeC/solution_1"
grep -qx "sieve.c" "$tmp/out" || fail "list PrimeC/solution_1 does not show sieve.c"
$R list PrimeC > "$tmp/out" || fail "list refused PrimeC"
grep -qx "solution_1/" "$tmp/out" || fail "list PrimeC does not show the folder solution_1/"

# what is not in the pin is refused, and does not fall back to Linux
refuse "a file that is not in the drag race's tree" "is not a file" $R read PrimeC/solution_1/no-such-file.c 1 2
refuse "a Linux path" "is not a file" $R read drivers/tty/serial/8250/8250_early.c 1 2
refuse "a Berkeley path" "is not a file" $R read usr/src/bin/cat/cat.c 1 2
refuse "a directory read as a file" "is not a file" $R read PrimeC 1 2
refuse "lines past the end" "past the end" $R read "$F" 201 $((total + 1))
refuse "a directory that is not there" "not a directory" $R list PrimeC/no-such-dir
refuse "a file listed as a directory" "not a directory" $R list "$F"
refuse "a source it does not have" "unknown source" sh jig/reader --source plan9 pin
READER_PIN=0000000000000000000000000000000000000000 $R pin > /dev/null 2> "$tmp/err"; rc=$?
[ $rc -eq 2 ] && grep -q "lacks the pinned commit" "$tmp/err" || fail "a commit the clone lacks gave exit $rc, not 'cannot run here'"

# the questions that need Linux's cscope index say so
for q in find callers calls; do
	refuse "$q about the drag race" "only Linux" $R $q main
done
echo "Primes at $(printf '%.12s' "$pin"): $F read byte for byte ($total lines), a range from the middle, list, eight refusals and a commit the clone lacks, and find, callers and calls say Linux-only"
