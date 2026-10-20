#!/bin/sh
# Probe for TST.builder.*@1: jig/aios on a flavor whose petition names catalogue
# numbers that have no entry.  In a throwaway copy of the repo, the hand-written
# draft in tests/fixtures/draft/petition-good.toml becomes petition 0012 and a flavor
# "cat" is made for it.  Two modes, one per test:
#   missing-list   the build stops with status 3, writes out/cat/missing.txt, and the
#                  file is the expected one, the same on a second run
#   no-guess       the list holds exactly the numbers that have no entry: none that
#                  have one, none the petition does not name; once the entries exist
#                  the list is empty and a stale one is removed; a flavor that was
#                  missing nothing (hello) is not given one
# Exit 0 is green, 1 is red.
set -u
cd "$(dirname "$0")/../.."
mode=${1:-}
want=tests/expected/builder.missing-list.out
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
tar --exclude=./out --exclude=./.git -cf - . | tar -xf - -C "$t"
cp tests/fixtures/draft/petition-good.toml "$t/petitions/0012.toml"
printf 'name      = "cat"\nmachine   = "MACHINE.serial-only@1"\npetitions = ["0012"]\n' > "$t/flavors/cat.toml"
fail() { echo "$*"; exit 1; }
build() { (cd "$t" && sh jig/aios build cat > "$t/stdout" 2> "$t/stderr"); }

case "$mode" in
missing-list)
	build; rc=$?
	[ $rc -eq 3 ] || fail "a build that cannot finish exited with status $rc, not 3"
	[ -f "$t/out/cat/missing.txt" ] || fail "the build stopped but wrote no out/cat/missing.txt"
	cmp -s "$t/out/cat/missing.txt" "$want" || fail "the list is not the expected one: $(tr '\n' '|' < "$t/out/cat/missing.txt")"
	cp "$t/out/cat/missing.txt" "$t/first.txt"
	build; rc=$?
	[ $rc -eq 3 ] && cmp -s "$t/out/cat/missing.txt" "$t/first.txt" || fail "a second run gave status $rc and a different list"
	echo "the build of cat stopped with status 3 and wrote the 3 missing numbers, the same both times" ;;
no-guess)
	(cd "$t" && sh jig/aios missing cat) > "$t/list" 2> /dev/null
	cmp -s "$t/list" "$want" || fail "the list is not exactly the numbers with no entry: $(tr '\n' '|' < "$t/list")"
	for have in PUR.0011@1 KEY.chris@1 CAP.tty.write@1 CAP.fs.read@1 PART.loader@9; do
		! grep -q "^$have " "$t/list" || fail "$have has an entry and is listed as missing"
	done
	[ "$(wc -l < "$t/list")" -eq 3 ] || fail "the list has $(wc -l < "$t/list") lines, not the 3 numbers the petition names that have no entry"
	: > "$t/index/part/cat@1.toml"; : > "$t/specs/cat.1@1.toml"; : > "$t/tests/cat.prints-file@1.toml"
	mkdir -p "$t/out/cat"; echo "PART.cat@1  part not in the catalogue" > "$t/out/cat/missing.txt"
	(cd "$t" && sh jig/aios missing cat) > "$t/list" 2> /dev/null
	[ ! -s "$t/list" ] || fail "numbers are still listed once their entries exist: $(tr '\n' '|' < "$t/list")"
	build; rc=$?
	[ $rc -ne 3 ] || fail "a flavor missing nothing still stopped with status 3"
	[ ! -e "$t/out/cat/missing.txt" ] || fail "a flavor missing nothing kept a stale list"
	(cd "$t" && sh jig/aios missing hello) > "$t/list" 2> /dev/null
	[ ! -s "$t/list" ] || fail "hello, missing nothing, was given a list"
	echo "exactly the 3 numbers with no entry; none once they have one; a stale list removed; hello given none" ;;
*)	fail "unknown mode '$mode'" ;;
esac
