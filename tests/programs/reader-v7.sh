#!/bin/sh
# Probe for TST.reader.v7-pin@1: jig/reader asked about Unix Version 7, against a
# real clone of the history repo at the commit in jig/v7.pin.  Checks that the
# lines it hands back are V7's lines byte for byte, taken a different way from
# the reader (out of an archive); that it answers from the pin and nothing else,
# so a Linux path is refused and does not fall back to Linux; and that find,
# callers and calls, which need Linux's cscope index, say they are Linux-only.
# Exit 0 is green, 1 is red, 2 means this host cannot run it (no clone, no pin).
set -u
cd "$(dirname "$0")/../.."
V7=${V7:-$PWD/../unix-history-repo}; export V7
[ -d "$V7/.git" ] || { echo "no V7 clone at $V7 (set V7=)"; exit 2; }
pin=$(sed -n 's/^commit *= *"\([0-9a-f]*\)".*/\1/p' jig/v7.pin)
[ -n "$pin" ] || { echo "no commit in jig/v7.pin"; exit 2; }
git -C "$V7" cat-file -e "$pin^{commit}" 2> /dev/null || { echo "the clone at $V7 lacks the pinned commit $pin"; exit 2; }
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
R="sh jig/reader --source v7"
F=usr/src/cmd/cat.c
fail() { echo "$*"; exit 1; }

# refuse WHAT FRAGMENT COMMAND...: exit status 1 and FRAGMENT on stderr, nothing on stdout
refuse() {
	what=$1; frag=$2; shift 2
	"$@" > "$tmp/out" 2> "$tmp/err"; rc=$?
	[ $rc -eq 1 ] || fail "$what: exit status $rc, not a refusal"
	grep -q -- "$frag" "$tmp/err" || fail "$what: refused, but the reason says '$(head -1 "$tmp/err")', not '$frag'"
	[ ! -s "$tmp/out" ] || fail "$what: refused, but still printed something"
}

# the pin is the V7 pin, not Linux's
[ "$($R pin)" = "$pin" ] || fail "pin says '$($R pin)', not the commit in jig/v7.pin"

# the lines are V7's lines, byte for byte, taken a different way: out of an archive
git -C "$V7" archive "$pin" "$F" | tar -xO > "$tmp/want"
total=$(awk 'END { print NR }' "$tmp/want")
[ "$total" -gt 10 ] || fail "the archived $F has only $total lines"
$R read "$F" 1 "$total" > "$tmp/got" || fail "read refused lines that are there"
sed "s|^$F:[0-9]*: ||" "$tmp/got" > "$tmp/text"
cmp -s "$tmp/text" "$tmp/want" || fail "the lines read from $F differ from the archive of the pin"
awk -v f="$F" '{ if ($0 !~ "^" f ":" NR ": ") bad = 1 } END { exit bad }' "$tmp/got" || fail "the lines are not numbered 1 to $total"
$R read "$F" 5 9 > "$tmp/got"
[ "$(sed -n 5p "$tmp/want")" = "$(sed -n "1s|^$F:5: ||p" "$tmp/got")" ] || fail "a range from the middle starts at the wrong line"
[ "$(wc -l < "$tmp/got")" -eq 5 ] || fail "a range of 5 lines came back as $(wc -l < "$tmp/got")"

# list is the pinned tree
$R list usr/src/cmd > "$tmp/out" || fail "list refused a directory that is there"
git -C "$V7" ls-tree --name-only "$pin:usr/src/cmd" | grep -x cat.c > /dev/null || fail "the archive has no cat.c in usr/src/cmd"
grep -qx "cat.c" "$tmp/out" || fail "list usr/src/cmd does not show cat.c"
grep -qx "$(git -C "$V7" ls-tree "$pin:usr/src/cmd" | awk -F'\t' '$1 ~ / tree / { print $2 "/"; exit }')" "$tmp/out" || fail "a folder in usr/src/cmd is not shown ending in /"

# what is not in the pin is refused, and does not fall back to Linux
refuse "a file that is not in V7" "is not a file" $R read usr/src/cmd/no-such-file.c 1 2
refuse "a Linux path" "is not a file" $R read drivers/tty/serial/8250/8250_early.c 1 2
refuse "a directory read as a file" "is not a file" $R read usr/src/cmd 1 2
refuse "lines past the end" "past the end" $R read "$F" 1 $((total + 1))
refuse "a directory that is not there" "not a directory" $R list usr/src/no-such-dir
refuse "a file listed as a directory" "not a directory" $R list "$F"
refuse "a source it does not have" "unknown source" sh jig/reader --source plan9 pin
READER_PIN=0000000000000000000000000000000000000000 $R pin > /dev/null 2> "$tmp/err"; rc=$?
[ $rc -eq 2 ] && grep -q "lacks the pinned commit" "$tmp/err" || fail "a commit the clone lacks gave exit $rc, not 'cannot run here'"

# the questions that need Linux's cscope index say so
for q in find callers calls; do
	refuse "$q about V7" "only Linux" $R $q main
done
echo "V7 at $(printf '%.12s' "$pin"): $F read byte for byte ($total lines), a range from the middle, list, seven refusals and a commit the clone lacks, and find, callers and calls say Linux-only"
