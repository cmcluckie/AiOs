#!/bin/sh
# Probe for TST.libc.pieces@1: jig/libc builds musl from the commit in jig/musl.pin
# out of a real clone (MUSL=, default ../musl), and lists what a small program
# takes from it.  Checks the version built is the pin's, that two musl files with
# the same name (src/malloc/free.c and src/malloc/mallocng/free.c) stay two pieces,
# that the list for tests/fixtures/libc/hello.c is the expected one and the same
# twice, and that a pin the clone lacks means "cannot run here", not a guess.
# Exit 0 green, 1 red, 2 this host cannot run it (no clone, no 32-bit compiler).
set -u
cd "$(dirname "$0")/../.."
MUSL=${MUSL:-$PWD/../musl}; export MUSL
[ -d "$MUSL/.git" ] || { echo "no clone of musl at $MUSL (set MUSL=)"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
fail() { echo "$*"; exit 1; }
export LIBC_OUT="$t/musl"
sh jig/libc build > "$t/build" 2>&1; rc=$?
[ $rc -eq 2 ] && { cat "$t/build"; exit 2; }
[ $rc -eq 0 ] || fail "the build failed: $(tail -1 "$t/build")"
grep -q "^built musl 1.2.4 for i386" "$t/build" || fail "the build is not musl 1.2.4, the pin's version: $(tail -1 "$t/build")"
ar t "$t/musl/lib/libc.a" > "$t/members"
grep -qx "src@malloc@free.o" "$t/members" && grep -qx "src@malloc@mallocng@free.o" "$t/members" || fail "src/malloc/free.c and src/malloc/mallocng/free.c did not stay two pieces"
${CC:-gcc} -m32 -O2 -nostdinc -fno-pie -fno-stack-protector -I "$t/musl/include" -c -o "$t/hello.o" tests/fixtures/libc/hello.c || fail "the fixture did not compile against the build's headers"
${CC:-gcc} -m32 -O2 -ffreestanding -fno-pie -c -o "$t/div.o" pool/PART.divide64@1/divide64.c || exit 2
sh jig/libc pieces "$t/hello.o" "$t/div.o" > "$t/p1" 2> "$t/err" || fail "pieces refused a program that links: $(tail -1 "$t/err")"
cmp -s "$t/p1" tests/expected/libc.hello-pieces.txt || fail "the pieces are not the expected $(wc -l < tests/expected/libc.hello-pieces.txt): $(diff "$t/p1" tests/expected/libc.hello-pieces.txt | head -3 | tr '\n' ' ')"
sh jig/libc pieces "$t/hello.o" "$t/div.o" > "$t/p2" 2> /dev/null
cmp -s "$t/p1" "$t/p2" || fail "the same program gave two different lists"
# a pin the clone does not have: cannot run here, exit 2, and nothing built
mkdir -p "$t/repo/jig"; cp jig/libc "$t/repo/jig/"; printf 'commit = "%s"\n' 0000000000000000000000000000000000000000 > "$t/repo/jig/musl.pin"
LIBC_OUT="$t/other" sh "$t/repo/jig/libc" build > /dev/null 2> "$t/err"; rc=$?
[ $rc -eq 2 ] && grep -q "lacks the pinned commit" "$t/err" || fail "a pin the clone lacks gave exit $rc: $(tail -1 "$t/err")"
[ ! -e "$t/other/lib/libc.a" ] || fail "a pin the clone lacks still built something"
echo "musl 1.2.4 built from the pin, the two free.c files kept apart, hello's $(wc -l < "$t/p1") pieces as expected and the same twice, and a pin the clone lacks refused before anything was built"
