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
#   flags          hello's catalogue entry is given cflags: its compile gets exactly
#                  those, after the builder's own, -include= becomes -include and the
#                  header, and the loader gets none; a flag outside the allowed forms
#                  is refused before anything is compiled
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
flags)
	command -v gcc > /dev/null || { echo "no gcc"; exit 2; }
	printf '#!/bin/sh\nprintf "%%s\\n" "$*" >> "%s/cc.log"\nexec gcc "$@"\n' "$t" > "$t/cc.sh"
	e="$t/index/part/hello@1.toml"
	printf '#define FROM_HEADER 1\n' > "$t/pool/PART.hello@1/flags.h"
	printf 'cflags        = ["-std=gnu89", "-DFROM_ENTRY", "-D__P(x)=x", "-include=pool/PART.hello@1/flags.h"]\n' >> "$e"
	(cd "$t" && CC="sh $t/cc.sh" sh jig/aios build hello > "$t/stdout" 2> "$t/stderr") || fail "hello with cflags did not build: $(tail -1 "$t/stderr")"
	grep -- " -c -o out/hello/hello.o " "$t/cc.log" > "$t/prog" || fail "the compile of hello is not in the compiler's log"
	sed 's/ -c -o out\/hello\/hello.o .*//' "$t/prog" | grep -q -- " -Werror -std=gnu89 -DFROM_ENTRY -D__P(x)=x -include pool/PART.hello@1/flags.h$" || fail "hello was not compiled with exactly its flags after the builder's own: $(head -1 "$t/prog")"
	grep -- " -c -o out/hello/" "$t/cc.log" | grep -v -- " -c -o out/hello/hello.o " > "$t/others"
	[ -s "$t/others" ] || fail "the loader's compiles are not in the compiler's log"
	grep -q -- "FROM_ENTRY" "$t/others" && fail "the loader got hello's flags: $(grep -- FROM_ENTRY "$t/others" | head -1)"
	for bad in '-fplugin=evil.so' '-DX=$(id)' '-include=../../etc/passwd' '-std=c99 -o'; do
		cp "$t/index/part/hello@1.toml" "$t/keep"; : > "$t/cc.log"
		sed -i '/^cflags/d' "$e"; printf 'cflags        = ["%s"]\n' "$bad" >> "$e"
		(cd "$t" && CC="sh $t/cc.sh" sh jig/aios build hello > /dev/null 2> "$t/stderr"); rc=$?
		[ $rc -eq 1 ] || fail "the flag '$bad' gave exit status $rc, not a refusal"
		grep -q "PART.hello@1" "$t/stderr" && grep -qF -- "$(printf '%s' "$bad" | cut -c1-8)" "$t/stderr" || fail "the flag '$bad' was refused without naming the program and the flag: $(tail -1 "$t/stderr")"
		[ ! -s "$t/cc.log" ] || fail "the flag '$bad' was refused only after something was compiled"
		cp "$t/keep" "$e"
	done
	echo "hello compiled with exactly its four flags after the builder's own, -include= handed over as -include and the header, nothing else given them, and four flags outside the allowed forms each refused before anything was compiled" ;;
*)	fail "unknown mode '$mode'" ;;
esac
