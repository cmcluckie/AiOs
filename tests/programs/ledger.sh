#!/bin/sh
# Probe for TST.ledger.human-only@1: a throwaway copy of the repo with one
# commit, then four changes, each checked by `jig/aios ledger`:
#   a test made easier (a longer timeout, a softer red_when), no decision: refused
#   the same, with a decision decided by a key that is not in constitution/keys: refused
#   the same, with a decision decided by KEY.chris@1: accepted
#   a change to a program's code only, no decision: accepted (code is the agents' to write)
set -u
cd "$(dirname "$0")/../.."
command -v git > /dev/null || { echo "no git"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
tar --exclude=./out --exclude=./.git -cf - . | tar -xf - -C "$t"
cd "$t"
g() { git -c user.name=probe -c user.email=probe@example.invalid -c commit.gpgsign=false "$@"; }
g init -q && g add -A && g commit -q -m base || { echo "could not make the copy"; exit 1; }
sed -i 's/^timeout_ms = 60000$/timeout_ms = 600000/; s/^red_when   = ".*"$/red_when   = "the editor never boots"/' tests/editor.typing@1.toml
out=$(sh jig/aios ledger 2>&1) && { echo "a test made easier with no decision was accepted"; exit 1; }
case "$out" in *"tests/editor.typing@1.toml changed; tests are human-only"*) ;; *) echo "refused, but not for the test: $out"; exit 1 ;; esac
mkdir -p decisions
printf 'id      = "9001"\ndate    = 2026-10-14\nkind    = "tests"\ntouches = ["tests/editor.typing@1.toml"]\nasked   = "loosen the test?"\noptions = ["yes", "no"]\nchose   = "yes"\nby      = "KEY.claude@1"\n' > decisions/9001.toml
out=$(sh jig/aios ledger 2>&1) && { echo "a decision by a key that is not a human's was accepted"; exit 1; }
sed -i 's/KEY.claude@1/KEY.chris@1/' decisions/9001.toml
out=$(sh jig/aios ledger 2>&1) || { echo "a decision by KEY.chris@1 was refused: $out"; exit 1; }
g add -A && g commit -q -m step
echo "/* a comment an agent added */" >> pool/PART.editor@1/editor.c
out=$(sh jig/aios ledger 2>&1) || { echo "a code change with no decision was refused: $out"; exit 1; }
echo "a test made easier was refused, a forged decision was refused, a human's decision let it through, code alone needed none"
