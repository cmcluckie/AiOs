#!/bin/sh
# Probe for TST.reader.*@1: jig/reader against a real clone of Linux at the
# pinned commit.  One mode per test:
#   refuses   every kind of question the reader must turn down, each with its reason
#   exact     the lines it hands back are Linux's lines, byte for byte
#   find      cscope's three questions on the pinned source, the same every time
#   bounded   no answer is longer than READER_MAX
# Exit 0 is green, 1 is red, 2 means this host cannot run it (no clone, no pin, no cscope).
set -u
cd "$(dirname "$0")/../.."
mode=${1:-}
LINUX=${LINUX:-$PWD/../linux}; export LINUX
[ -d "$LINUX/.git" ] || { echo "no Linux clone at $LINUX (set LINUX=)"; exit 2; }
pin=$(sed -n 's/^commit *= *"\([0-9a-f]*\)".*/\1/p' jig/linux.pin)
git -C "$LINUX" cat-file -e "$pin^{commit}" 2> /dev/null || { echo "the clone at $LINUX lacks the pinned commit $pin"; exit 2; }
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
R="sh jig/reader"
F=drivers/tty/serial/8250/8250_early.c
fail() { echo "$*"; exit 1; }

# refuse WHAT FRAGMENT COMMAND...: exit status 1 and FRAGMENT on stderr, nothing on stdout
refuse() {
	what=$1; frag=$2; shift 2
	"$@" > "$tmp/out" 2> "$tmp/err"; rc=$?
	[ $rc -eq 1 ] || fail "$what: exit status $rc, not a refusal"
	grep -q -- "$frag" "$tmp/err" || fail "$what: refused, but the reason says '$(head -1 "$tmp/err")', not '$frag'"
	[ ! -s "$tmp/out" ] || fail "$what: refused, but still printed something"
}

# the Linux file at the pin, taken a different way from the reader: out of an archive
archived() { git -C "$LINUX" archive "$pin" "$1" | tar -xO; }

case "$mode" in
refuses)
	refuse "a file that does not exist" "is not a file" $R read no/such/file.c 1 2
	refuse "a directory read as a file" "is not a file" $R read drivers/tty/serial 1 2
	refuse "lines past the end of the file" "past the end" $R read $F 200 300
	refuse "line 0" "FIRST must be 1" $R read $F 0 5
	refuse "a range written backwards" "before FIRST" $R read $F 10 5
	refuse "a line number that is a word" "whole number" $R read $F one 5
	refuse "a directory that does not exist" "not a directory" $R list no/such/dir
	refuse "a file listed as a directory" "not a directory" $R list $F
	refuse "a check outside the file" "past the end" $R check $F 220 230
	refuse "a name with a semicolon in it" "not a function, macro or variable name" $R find 'a;b'
	refuse "a question it does not know" "unknown question" $R frobnicate
	$R check $F 59 62 > "$tmp/out" 2>&1 || fail "check refused lines that are there"
	[ "$(cat "$tmp/out")" = "ok $F 59-62" ] || fail "check said '$(cat "$tmp/out")'"
	READER_PIN=0000000000000000000000000000000000000000 $R pin > /dev/null 2> "$tmp/err"; rc=$?
	[ $rc -eq 2 ] && grep -q "lacks the pinned commit" "$tmp/err" || fail "a commit the clone lacks gave exit $rc, not 'cannot run here'"
	echo "turned down: a missing file, a directory read as a file, lines past the end, line 0, a backwards range, a word for a number, a missing directory, a file listed as a directory, a check outside the file, a name with a semicolon, an unknown question, and a commit the clone lacks" ;;
exact)
	[ "$($R pin)" = "$pin" ] || fail "pin says $($R pin), jig/linux.pin says $pin"
	$R read $F 59 62 > "$tmp/out" || fail "read 59-62 was refused"
	[ "$(wc -l < "$tmp/out")" -eq 4 ] || fail "read 59-62 gave $(wc -l < "$tmp/out") lines, not 4"
	[ "$(sed -n 1p "$tmp/out")" = "$F:59: static void serial8250_early_out(struct uart_port *port, int offset, int value)" ] || fail "line 59 is '$(sed -n 1p "$tmp/out")'"
	[ "$(sed -n 3p "$tmp/out")" = "$(printf '%s:61: \toffset <<= port->regshift;' "$F")" ] || fail "line 61 is '$(sed -n 3p "$tmp/out")'"
	archived $F | awk -v f=$F 'NR <= 200 { printf "%s:%d: %s\n", f, NR, $0 }' > "$tmp/want"
	$R read $F 1 200 > "$tmp/out" || fail "read 1-200 was refused"
	cmp -s "$tmp/out" "$tmp/want" || fail "lines 1-200 differ from the archive's: $(diff "$tmp/out" "$tmp/want" | head -1)"
	last=$(archived $F | awk 'END { print NR }')
	$R read $F "$last" "$last" > "$tmp/out" || fail "the last line ($last) was refused"
	[ "$(archived $F | tail -n 1)" = "$(sed "s|^$F:$last: ||" "$tmp/out")" ] || fail "the last line is wrong"
	$R list drivers/tty/serial/8250 | grep -qx '8250_early.c' || fail "list missed 8250_early.c"
	$R list arch/x86 | grep -qx 'boot/' || fail "list did not mark boot as a folder"
	$R list . | grep -qx 'drivers/' || fail "list . missed drivers/"
	$R ports $F 55 100 > "$tmp/out" || fail "ports was refused"
	[ "$(cat "$tmp/out")" = "$(printf '%s:78: \t\toutb(value, port->iobase + offset);' "$F")" ] || fail "ports 55-100 said '$(cat "$tmp/out")'"
	echo "the 4 lines asked for, the first 200 lines of $F and its last line ($last) equal the archive's, list marks folders, ports finds the one outb in 55-100" ;;
find)
	command -v cscope > /dev/null || { echo "cscope is not installed"; exit 2; }
	export READER_DIRS="drivers/tty/serial/8250 include/uapi/linux/serial_reg.h" READER_CACHE="$tmp/cache"
	ask() { echo "== $*"; $R "$@" || fail "$* was refused: $($R "$@" 2>&1 | head -1)"; }
	{ ask find serial8250_early_out; ask callers serial8250_early_out; ask calls early_serial8250_write; } > "$tmp/got" 2>&1
	{ ask find serial8250_early_out; ask callers serial8250_early_out; ask calls early_serial8250_write; } > "$tmp/again" 2>&1
	cmp -s "$tmp/got" "$tmp/again" || fail "the same questions gave two different answers"
	[ -z "${AIOS_KEEP:-}" ] || cp "$tmp/got" "$AIOS_KEEP"
	cmp -s "$tmp/got" tests/expected/reader.find@1.out || fail "the answers differ from tests/expected/reader.find@1.out: $(diff "$tmp/got" tests/expected/reader.find@1.out | head -2 | tr '\n' ' ')"
	refuse "a name nothing defines" "nothing for no_such_function_xyz" $R find no_such_function_xyz
	READER_MAX=3 $R callers serial8250_early_out > "$tmp/out" 2> "$tmp/err" || fail "callers with a small READER_MAX was refused"
	[ "$(wc -l < "$tmp/out")" -eq 3 ] || fail "READER_MAX=3 gave $(wc -l < "$tmp/out") lines of callers"
	grep -q "more not shown" "$tmp/err" || fail "a cut-short answer did not say so"
	echo "find, callers and calls match the recorded answers twice running, an unknown name is refused, and a cut-short answer says how much it left out" ;;
bounded)
	refuse "201 lines at once" "at most 200" $R read $F 1 201
	READER_MAX=3 $R read $F 1 3 > "$tmp/out" || fail "3 lines with READER_MAX=3 was refused"
	[ "$(wc -l < "$tmp/out")" -eq 3 ] || fail "3 lines asked for, $(wc -l < "$tmp/out") given"
	READER_MAX=3 refuse "4 lines with READER_MAX=3" "at most 3" $R read $F 1 4
	READER_MAX=5 $R list drivers > "$tmp/out" || fail "list was refused"
	[ "$(wc -l < "$tmp/out")" -eq 5 ] || fail "list drivers with READER_MAX=5 gave $(wc -l < "$tmp/out") entries"
	echo "200 lines is the most read will give, READER_MAX=3 stops a fourth, and list stops at the cap" ;;
*)	fail "tests/programs/reader.sh needs a mode: refuses, exact, find or bounded" ;;
esac
