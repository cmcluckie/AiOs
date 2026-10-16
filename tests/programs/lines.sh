#!/bin/sh
# Probe for TST.referee.verb-noun@1: seven lists through `pool/AGT.referee@1/referee lines`.
# One is a legal list and must pass.  Six are each wrong in one way and must
# each be refused under rule 13.
set -u
cd "$(dirname "$0")/../.."
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
ok() { sh pool/AGT.referee@1/referee lines "$1" > /dev/null 2>&1 || { echo "a legal list was refused: $(sh pool/AGT.referee@1/referee lines "$1" 2>&1)"; exit 1; }; }
no() { out=$(sh pool/AGT.referee@1/referee lines "$1" 2>&1) && { echo "accepted, and it should not have been: $2"; exit 1; }; case "$out" in *"rule 13"*) ;; *) echo "refused, but not under rule 13: $out"; exit 1 ;; esac; }
printf '1. lift PART.fs.minix@2\n2. rerun TST.editor.undo@1\n3. ask VERB.measure\n' > "$t/a"; ok "$t/a"
printf 'We should probably lift the Minix filesystem.\n' > "$t/b"; no "$t/b" "prose"
printf '1. install PART.fs.minix@2\n' > "$t/c"; no "$t/c" "a verb not in the constitution"
printf '1. lift PART.nothing@9\n' > "$t/d"; no "$t/d" "a noun not in the catalogue"
printf '1. lift PART.fs.minix@2 because it is good\n' > "$t/e"; no "$t/e" "words after the noun"
printf '1. lift PART.fs.minix@2\n3. pin PART.hello@1\n' > "$t/f"; no "$t/f" "a skipped number"
printf '1. ask VERB.lift\n2. lift PART.fs.minix@2\n' > "$t/g"; no "$t/g" "a verb asked for and used in the same list"
echo "one legal list accepted; prose, an unknown verb, an unknown noun, extra words, a skipped number and an asked-and-used verb each refused under rule 13"
