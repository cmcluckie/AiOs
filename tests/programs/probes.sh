#!/bin/sh
# Probe for TST.referee.probes-granted@1: rule 22, the one door for test
# programs (decision 0065).  The referee (probes) passes the test programs as
# they are; in a throwaway copy it refuses:
#   catfiles.c said to boot on cat, whose petition does not ask to write
#   nowrite.c with its "Expects refusal" line taken off
#   a program on cat that expects CAP.tty.write refused, which cat asks for
#   pastend.c with no "Boots on" line
#   pastend.c said to boot on editor, while pastend.sh boots it on cat alone
# Every refusal names rule 22 and the program.  Exit 0 green, 1 red.
set -u
cd "$(dirname "$0")/../.."
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
fail() { echo "$*"; exit 1; }
sh pool/AGT.referee@1/referee probes > "$t/out" 2>&1 || fail "the test programs as they are: $(tail -1 "$t/out")"
copy() { rm -rf "$t/r"; mkdir "$t/r"; tar --exclude=./out --exclude=./.git -cf - . | tar -xf - -C "$t/r"; }
refused() {	# WHAT PROGRAM WORDS...
	what=$1; p=$2; shift 2
	(cd "$t/r" && sh pool/AGT.referee@1/referee probes > "$t/out" 2>&1) && fail "passed $what"
	for w in "rule 22" "$p" "$@"; do grep -qF -- "$w" "$t/out" || fail "the refusal of $what does not name '$w': $(tail -1 "$t/out")"; done
}
copy; sed -i 's/Boots on: editor\./Boots on: cat./' "$t/r/tests/programs/catfiles.c"
grep -q 'Boots on: cat\.' "$t/r/tests/programs/catfiles.c" || fail "catfiles.c does not say it boots on editor"
refused "catfiles.c on cat" catfiles.c cat save CAP.fs.write
copy; sed -i '/Expects refusal/d' "$t/r/tests/programs/nowrite.c"
refused "nowrite.c expecting nothing refused" nowrite.c cat save
copy; sed 's/Expects refusal: CAP.fs.write\./Expects refusal: CAP.fs.write, CAP.tty.write./' tests/programs/nowrite.c > "$t/r/tests/programs/nowrite.c"
refused "a granted call expected refused" nowrite.c CAP.tty.write
copy; sed -i '/Boots on:/d' "$t/r/tests/programs/pastend.c"
refused "pastend.c with no Boots on line" pastend.c "Boots on"
copy; sed -i 's/Boots on: cat\./Boots on: editor./' "$t/r/tests/programs/pastend.c"
refused "pastend.c said to boot where its script does not boot it" pastend.c pastend.sh cat
echo "rule 22 held: the test programs as they are; refused five ways"
