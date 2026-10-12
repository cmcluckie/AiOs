# Gen zero.  Every package here has a row in design.md's tool table; jig/tools.sh checks.
{ pkgs, ... }:
{
  imports = [ ./hardware-configuration.nix ];
  system.stateVersion = "26.05";
  networking.hostName = "jig";
  boot.loader.systemd-boot.enable = true;
  boot.loader.efi.canTouchEfiVariables = true;
  boot.kernelModules = [ "kvm-amd" ];        # the box is a Ryzen 9 5900X
  virtualisation.hypervGuest.enable = true;  # Hyper-V's own drivers: disk, network, clean shutdown
  nix.settings.experimental-features = [ "nix-command" "flakes" ];
  nix.channel.enable = false;
  documentation.enable = false;
  environment.defaultPackages = [ ];
  environment.systemPackages = with pkgs; [ git qemu_kvm gcc_multi strace ];  # gcc_multi: the loader is built -m32
  services.openssh.enable = true;
  services.openssh.settings.PasswordAuthentication = false;
  users.users.chris = {
    isNormalUser = true;
    extraGroups = [ "wheel" "kvm" ];
    openssh.authorizedKeys.keys = [ "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIFWosB2gEpFJosmm8AOu2Po49JDATCzMU9Sp3VVqZSw9 chris.mcluckie@gmail.com" ];
  };
  security.sudo.wheelNeedsPassword = false;
}
