#!/bin/sh
# Plan item 2.3: every package the jig installs maps to a row in design.md's tool table.
# Run on the jig from the repo root: sh jig/tools.sh
# Prints one line per package: the row it maps to, or "no row".  Exit 1 if any has none.
set -eu
repo=$(cd "$(dirname "$0")/.." && pwd)
table=$(sed -n '/^| Tool |/,/^$/p' "$repo/design.md")
# What configuration.nix adds, read from the file itself.  Base NixOS (coreutils,
# systemd and friends) is the "NixOS minimal" row; evaluating the whole system's
# package list would name all of it, so only the added list is checked here.
names=$(sed -n 's/.*environment\.systemPackages *= *with pkgs; *\[\([^]]*\)\].*/\1/p' "$repo/jig/configuration.nix" | tr ' ' '\n' | grep . | sort -u)
[ -n "$names" ] || { echo "no environment.systemPackages list found in jig/configuration.nix"; exit 1; }
rc=0
for n in $names; do
  case "$n" in
    gcc*|binutils*) key="C compiler" ;;
    openssh) key="SSH" ;;
    qemu*) key="QEMU" ;;
    bash|dash) key="Shell" ;;
    git) key="Git" ;;
    *) key=$n ;;
  esac
  if printf '%s\n' "$table" | grep -qi "^| [^|]*$key"; then
    printf '%-24s -> %s\n' "$n" "$key"
  else
    printf '%-24s -> no row\n' "$n"; rc=1
  fi
done
exit $rc
