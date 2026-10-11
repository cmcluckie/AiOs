#!/bin/sh
# Plan item 2.3: every package the jig installs maps to a row in design.md's tool table.
# Run on the jig from the repo root: sh jig/tools.sh
# Prints one line per package: the row it maps to, or "no row".  Exit 1 if any has none.
set -eu
repo=$(cd "$(dirname "$0")/.." && pwd)
table=$(sed -n '/^| Tool |/,/^$/p' "$repo/design.md")
names=$(nix eval --raw "$repo/jig#nixosConfigurations.jig.config.environment.systemPackages" \
  --apply 'ps: builtins.concatStringsSep "\n" (map (p: p.pname or p.name) ps)' | sort -u)
# Base NixOS (coreutils, systemd and friends) is the "NixOS minimal" row and is not listed
# in systemPackages; only what configuration.nix adds is checked here.
rc=0
for n in $names; do
  case "$n" in
    gcc*|binutils*) key="C compiler" ;;
    openssh) key="SSH" ;;
    qemu*) key="QEMU" ;;
    bash|dash) key="Shell" ;;
    *) key=$n ;;
  esac
  if printf '%s\n' "$table" | grep -qi "^| [^|]*$key"; then
    printf '%-24s -> %s\n' "$n" "$key"
  else
    printf '%-24s -> no row\n' "$n"; rc=1
  fi
done
exit $rc
