#!/bin/sh
# Probe for TST.referee.artifact@1: rule 11, the built kernel held to the
# petitions (decision 0065).  The builder hands every linked kernel to the
# referee (artifact); in a throwaway copy of the repo:
#   calc builds as it is, its one extra (CAP.mem.alloc@1) listed
#   with that line taken off constitution/extras.toml, calc is refused
#   with an extra listed for hello that its kernel does not have, hello is refused
#   with CAP.fs.write@2 no longer switched off in PART.fs.minix@2, cat is refused
#   with the write code forced into cat's kernel, cat is refused, naming minix_save
#   with PART.loader@10 no longer listing CAP.fs.write@2 in its switches, cat is
#   refused by the builder, naming the kernel, not by the compiler
# Every refusal names rule 11, the flavor and the capability.
# Exit 0 green, 1 red.
set -u
cd "$(dirname "$0")/../.."
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
fail() { echo "$*"; exit 1; }
copy() {	# a fresh copy in $t/r, with cat's musl carried over so it is not rebuilt
	rm -rf "$t/r"; mkdir "$t/r"
	tar --exclude=./out --exclude=./.git -cf - . | tar -xf - -C "$t/r"
	if [ -f out/cat/musl/libc.a ]; then mkdir -p "$t/r/out/cat"; cp -r out/cat/musl "$t/r/out/cat/"; fi
}
build() { (cd "$t/r" && sh jig/aios build "$1" > "$t/stdout" 2> "$t/stderr"); }
refused() {	# FLAVOR WORDS...: the build of FLAVOR fails and its refusal names each word
	f=$1; shift
	build "$f" && fail "$f built; it should have been refused ($*)"
	for w in "rule 11" "$f" "$@"; do grep -qF -- "$w" "$t/stderr" || fail "the refusal of $f does not name '$w': $(tail -1 "$t/stderr")"; done
}
grep -q 'flavor = "calc"' constitution/extras.toml 2> /dev/null || fail "constitution/extras.toml lists nothing for calc"

copy
build calc || fail "calc does not build as it is: $(tail -1 "$t/stderr")"
awk 'BEGIN { RS = ""; ORS = "\n\n" } !/flavor = "calc"/' constitution/extras.toml > "$t/r/constitution/extras.toml"
refused calc CAP.mem.alloc@1

copy
printf '\n[[extra]]\nflavor = "hello"\ncap    = "CAP.time.read@1"\nwhy    = "made up by the probe"\n' >> "$t/r/constitution/extras.toml"
refused hello CAP.time.read@1

copy
sed -i '/^off_unless_asked/d' "$t/r/index/part/fs.minix@2.toml"
refused cat CAP.fs.write@2

copy
sed -i 's/^#ifdef GRANT_FS_WRITE$/#if 1/' "$t/r/lifted/PART.fs.minix@2/minix.c"
grep -q '^#if 1$' "$t/r/lifted/PART.fs.minix@2/minix.c" || fail "PART.fs.minix@2 has no #ifdef GRANT_FS_WRITE to force on"
refused cat CAP.fs.write@2 minix_save

copy
sed -i '/^switches/d' "$t/r/index/part/loader@10.toml"
refused cat CAP.fs.write@2 PART.loader@10

echo "rule 11 held: calc as it is; refused a taken-off extra, a stale extra, an unswitched part, forced-in write code and a kernel that cannot switch"
