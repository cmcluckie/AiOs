#!/bin/sh
# Run as root from the NixOS minimal ISO inside the jig VM, with this jig/ directory
# copied alongside (scp from the Windows box).  Partitions with labels so that
# hardware-configuration.nix is true as written, then installs from the flake.
set -eu
disk=${1:-/dev/sda}
here=$(cd "$(dirname "$0")" && pwd)
parted -s "$disk" mklabel gpt \
  mkpart ESP fat32 1MiB 513MiB set 1 esp on \
  mkpart nixos ext4 513MiB 100%
mkfs.fat -F 32 -n BOOT "${disk}1"
mkfs.ext4 -F -L nixos "${disk}2"
mount /dev/disk/by-label/nixos /mnt
mkdir -p /mnt/boot /mnt/etc/nixos
mount /dev/disk/by-label/BOOT /mnt/boot
cp "$here"/flake.nix "$here"/configuration.nix "$here"/hardware-configuration.nix /mnt/etc/nixos/
[ -f "$here/flake.lock" ] && cp "$here/flake.lock" /mnt/etc/nixos/
nixos-install --no-root-passwd --root /mnt \
  --option experimental-features "nix-command flakes" \
  --flake /mnt/etc/nixos#jig
# The lock file written by this install is the pin.  Copy it back into the repo and commit it.
cp /mnt/etc/nixos/flake.lock "$here/flake.lock" 2>/dev/null || true
echo "installed.  Commit jig/flake.lock.  Eject the ISO and reboot."
