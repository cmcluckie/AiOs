# Create the jig as a Gen 2 Hyper-V VM.  Run once, as Administrator, on the Windows box.
# Secure Boot off (systemd-boot is unsigned); nested virtualization on (KVM inside);
# dynamic memory off (nested virtualization requires it); no checkpoints.
param(
  [string]$Iso  = "$env:USERPROFILE\Downloads\nixos-minimal-x86_64-linux.iso",
  [string]$Vhd  = "C:\VMs\jig\jig.vhdx",
  [int64]$Memory = 8GB,
  [int]$Cpus = 4
)
$ErrorActionPreference = "Stop"
New-VM -Name jig -Generation 2 -MemoryStartupBytes $Memory -NewVHDPath $Vhd -NewVHDSizeBytes 64GB -SwitchName "Default Switch" | Out-Null
Set-VMFirmware -VMName jig -EnableSecureBoot Off
Set-VMProcessor -VMName jig -Count $Cpus -ExposeVirtualizationExtensions $true
Set-VMMemory -VMName jig -DynamicMemoryEnabled $false
Set-VM -VMName jig -CheckpointType Disabled -AutomaticStopAction ShutDown -AutomaticStartAction Nothing
Add-VMDvdDrive -VMName jig -Path $Iso
Set-VMFirmware -VMName jig -FirstBootDevice (Get-VMDvdDrive -VMName jig)
Start-VM jig
Write-Host "jig started from the ISO.  In the VM: sudo -i, then:"
Write-Host "  curl -L https://github.com/cmcluckie/aios/archive/main.tar.gz | tar xz"
Write-Host "  sh aios-main/jig/install.sh"
Write-Host "Then here: Stop-VM jig; Get-VMDvdDrive -VMName jig | Remove-VMDvdDrive; Start-VM jig"
