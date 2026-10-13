# CMP.timer.kvmclock@1 as built

Hand-derived from the source and the object file on 2026-10-12, pending the
generator (plan 7.1).  Measured with the gcc in Claude's workspace (not the
jig's).

## What it does
Asks KVM how fast the processor's time stamp counter ticks.  Looks for KVM's
signature, "KVMKVMKVM", in the hypervisor CPUID leaves, but only if CPUID says
a hypervisor is present at all.  If KVM offers its second paravirtual clock
(feature bit 3), lends KVM 32 bytes of its own memory through
MSR_KVM_SYSTEM_TIME_NEW, reads the multiplier and shift KVM writes there,
turns them into ticks per millisecond with Linux's pvclock_tsc_khz, and takes
the 32 bytes back by writing 0 to the same MSR.  Returns 0, having written
nothing, when there is no KVM or no such clock, and 0 when the 32 bytes come
back unfilled or mid-update.  Never touches the 8254.

## Every syscall or hardware access
CPUID leaves 1, 0x40000000 and up in steps of 0x100 until KVM is found (at
most 256), and 0x40000001.  Under KVM only: wrmsr to 0x4b564d01, twice.

## Every symbol it imports
None.

## Every byte it writes anywhere
The two MSR values: the 32 bytes' address with bit 0 set, then 0.  Its own 36
bytes of .bss (the 32 lent bytes and which MSR it used).  KVM writes the 32 lent
bytes; nothing else in memory is written.

## Size
1 exported function, 440 bytes of code in this build.  Source: 153 lines
of code, lifted apart from the glue marked "ours" (four typedefs, two #defines,
a 12-byte compare, kvmclock_khz itself); provenance in the file header.

## What was cut
Listed in kvmclock.c and origin.toml.
