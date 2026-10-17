#!/bin/sh
# Probe for TST.loader.partition@1 (SPEC.loader.13@1): for every flavor whose
# kernel is PART.loader@8, the parts in the built kernel are exactly the ones its
# petitions name, and what those depend on.  Worked out here from the petitions
# and the catalogue, not from the builder's own list, then checked against the
# functions actually in the kernel: every part that should be there has its
# functions in it, and no part that shouldn't be has a single one.
set -u
cd "$(dirname "$0")/../.."
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
K=index/part/loader@8.toml
lst() { sed -n "s/^$2 *= *\[\(.*\)\].*/\1/p" "$1" | tr ',' '\n' | sed 's/[" ]//g' | grep . || true; }
fld() { sed -n "s/^$2 *= *\"\([^\"]*\)\".*/\1/p" "$1" | head -1; }
cat_of() { n=${1#PART.}; echo "index/part/$n.toml"; }
deps_closed() {	# codes... -> the codes and everything they depend on
	s=$(printf '%s\n' "$@" | grep . | sort -u)
	while :; do
		n=$(printf '%s\n' $s $(for c in $s; do lst "$(cat_of "$c")" depends; done) | grep . | sort -u)
		[ "$n" = "$s" ] && break; s=$n
	done
	echo "$s"
}
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2"
universe=$(deps_closed $(lst $K depends) $(lst $K builds_with))
incs="-I pool/PART.loader@8"; for c in $universe; do incs="$incs -I $(fld "$(cat_of "$c")" source)"; done
# each part's own functions: its sources compiled alone
for c in $universe; do
	d=$(fld "$(cat_of "$c")" source); : > "$t/$c.syms"
	for s in "$d"*.c "$d"*.S; do [ -e "$s" ] || continue
		gcc $CF $incs -c -o "$t/o.o" "$s" 2> "$t/cc" || { echo "$c: $s does not build alone: $(head -1 "$t/cc")"; exit 1; }
		nm "$t/o.o" | awk '$2 == "T" {print $3}' >> "$t/$c.syms"
	done
	[ -s "$t/$c.syms" ] || { echo "$c has no functions of its own to look for"; exit 1; }
done
n=0
for ff in flavors/*.toml; do
	fl=$(fld "$ff" name)
	pets=$(lst "$ff" petitions)
	boots=$(for id in $pets; do awk '/^\[\[component\]\]/{c=""} /^code/{gsub(/[" ]/,"",$3);c=$3} /^role/{gsub(/[" ]/,"",$3); if($3=="boot")print c}' "petitions/$id.toml"; done | sort -t@ -k2,2n | tail -1)
	[ "$boots" = "PART.loader@8" ] || continue
	named=$(for id in $pets; do awk '/^\[\[component\]\]/{c=""} /^code/{gsub(/[" ]/,"",$3);c=$3} /^role/{gsub(/[" ]/,"",$3); if($3=="dependency")print c}' "petitions/$id.toml"; done | while read -r c; do printf '%s\n' $universe | grep -qx "$c" && echo "$c"; done)
	want=$(deps_closed $(lst $K depends) $named)
	# one version of a part: the newest wanted
	keep=""; for nm_ in $(for c in $want; do echo "${c%@*}"; done | sort -u); do keep="$keep $(for c in $want; do case $c in "$nm_"@*) echo "$c" ;; esac; done | sort -t@ -k2,2n | tail -1)"; done
	sh jig/aios build "$fl" > /dev/null 2> "$t/b" || { echo "flavor $fl does not build: $(head -1 "$t/b")"; exit 1; }
	nm "out/$fl/loader.elf" | awk '$2 == "T" || $2 == "t" {print $3}' | sort -u > "$t/k"
	for c in $universe; do
		hit=$(sort -u "$t/$c.syms" | comm -12 - "$t/k" | head -1)
		if printf '%s\n' $keep | grep -qx "$c"; then
			[ -n "$hit" ] || { echo "flavor $fl: $c is named and none of its functions is in the kernel"; exit 1; }
		else
			# another version of a kept part shares its function names; that is the kept one
			case " $(for k in $keep; do echo "${k%@*}"; done | tr '\n' ' ') " in *" ${c%@*} "*) continue ;; esac
			[ -z "$hit" ] || { echo "flavor $fl: $c is not named and $hit is in the kernel"; exit 1; }
		fi
	done
	n=$((n + 1)); echo "flavor $fl: $(echo $keep | wc -w) parts, exactly the named ones" >> "$t/ok"
done
[ $n -gt 0 ] || { echo "no flavor's kernel is PART.loader@8"; exit 1; }
cat "$t/ok" | tr '\n' ';' | sed 's/;$//; s/;/; /g'; echo
echo "$n flavors on PART.loader@8, each kernel exactly its petitions' parts"
