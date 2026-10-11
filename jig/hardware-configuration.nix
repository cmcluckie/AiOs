# Written by hand before the VM existed, so it uses disk labels, not UUIDs.
# jig/install.sh creates the labels.  Gen 2 Hyper-V: UEFI, SCSI disk via storvsc.
{ ... }:
{
  nixpkgs.hostPlatform = "x86_64-linux";
  boot.initrd.availableKernelModules = [ "hv_vmbus" "hv_storvsc" "sd_mod" "sr_mod" ];
  fileSystems."/" = { device = "/dev/disk/by-label/nixos"; fsType = "ext4"; };
  fileSystems."/boot" = { device = "/dev/disk/by-label/BOOT"; fsType = "vfat"; };
  swapDevices = [ ];
}
