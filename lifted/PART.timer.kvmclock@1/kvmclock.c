/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.timer.kvmclock@1: how fast does the processor's counter tick?  Asked of
 * the hypervisor, the way Linux does it under KVM: find KVM by its CPUID
 * signature, lend it 32 bytes through MSR_KVM_SYSTEM_TIME_NEW, read the rate
 * KVM writes there, take the 32 bytes back.  Exposes the rate in kHz, or 0 when
 * there is no KVM or it will not say; the loader then measures against the
 * 8254 instead (PART.timer.tsc-pit@2).  Requires CAP.hv.kvmclock.
 *
 * Lifted from Linux, commit a5ebb76233b79db01e82e061173cb73c6d2b5c6b (2026-10-10).
 * The machine-readable record is origin.toml beside this file; jig/upstream reads it.
 *   arch/x86/include/asm/cpuid/api.h      native_cpuid 26-37, __cpuid 60, cpuid 69-76,
 *                                         cpuid_eax 92-99, cpuid_ecx 110-117,
 *                                         for_each_possible_cpuid_base_hypervisor 192-193,
 *                                         cpuid_base_hypervisor 195-213
 *   arch/x86/kernel/kvm.c                 __kvm_cpuid_base 889-898,
 *                                         kvm_arch_para_features 916-919
 *   include/linux/kvm_para.h              kvm_para_has_feature 8-11
 *   arch/x86/include/uapi/asm/kvm_para.h  KVM_SIGNATURE 11, KVM_CPUID_FEATURES 18,
 *                                         KVM_FEATURE_CLOCKSOURCE2 25,
 *                                         MSR_KVM_SYSTEM_TIME_NEW 53
 *   arch/x86/include/asm/pvclock-abi.h    struct pvclock_vcpu_time_info 26-35
 *   arch/x86/include/asm/msr.h            __wrmsrq 78-84
 *   arch/x86/include/asm/div64.h          do_div 22-25, 30-41
 *   arch/x86/kernel/pvclock.c             pvclock_tsc_khz 27-37
 *   arch/x86/kernel/kvmclock.c            kvm_get_tsc_khz 139-143, kvm_register_clock
 *                                         190-201, kvmclock_disable 219-223, the
 *                                         MSR choice in kvmclock_init 318-326
 *
 * Cut, and why:
 *   boot_cpu_data.cpuid_level check     every processor this runs on has CPUID
 *   kvm_cpuid_base()'s cache            asked once, at boot
 *   __builtin_memcmp                    -fno-builtin would make it a call to a
 *                                       memcmp nobody provides; a 12-byte loop instead
 *   the old MSR_KVM_SYSTEM_TIME (0x12)  CLOCKSOURCE2 only; KVM has offered it since
 *                                       2010, and without it the 8254 answers
 *   per-CPU clock pages, the wall       one processor, one rate, read once; the
 *   clock, sched_clock, vsyscall        page is lent and taken back at boot
 *   slow_virt_to_phys()                 the loader's memory is identity-mapped
 *   the exception table on wrmsr        the MSR is written only after KVM said it exists
 *   setup_force_cpu_cap(), the tracepoint  no cpu caps, no tracing
 *   do_div's power-of-two branch        as in PART.timer.tsc-pit
 */

#include "kvmclock.h"

/* Glue, ours: what the kernel's headers would have provided. */
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned int u32;
typedef unsigned long long u64;
#define asm __asm__
#define X86_FEATURE_HYPERVISOR_BIT 31	/* CPUID 1, ECX bit 31: "running under a hypervisor" */

/* arch/x86/include/asm/cpuid/api.h */
static inline void native_cpuid(u32 *eax, u32 *ebx,
				u32 *ecx, u32 *edx)
{
	/* ecx is often an input as well as an output. */
	asm volatile("cpuid"
	    : "=a" (*eax),
	      "=b" (*ebx),
	      "=c" (*ecx),
	      "=d" (*edx)
	    : "0" (*eax), "2" (*ecx)
	    : "memory");
}

# define __cpuid native_cpuid

static inline void cpuid(u32 op,
			 u32 *eax, u32 *ebx,
			 u32 *ecx, u32 *edx)
{
	*eax = op;
	*ecx = 0;
	__cpuid(eax, ebx, ecx, edx);
}

static inline u32 cpuid_eax(u32 op)
{
	u32 eax, ebx, ecx, edx;

	cpuid(op, &eax, &ebx, &ecx, &edx);

	return eax;
}

static inline u32 cpuid_ecx(u32 op)
{
	u32 eax, ebx, ecx, edx;

	cpuid(op, &eax, &ebx, &ecx, &edx);

	return ecx;
}

/* Glue, ours: __builtin_memcmp(sig, signature, 12) without a memcmp */
static int sig_differs(const char *sig, const u32 *signature)
{
	const char *s = (const char *)signature;
	int i;

	for (i = 0; i < 12; i++)
		if (sig[i] != s[i])
			return 1;
	return 0;
}

#define for_each_possible_cpuid_base_hypervisor(function) \
	for (function = 0x40000000; function < 0x40010000; function += 0x100)

static inline u32 cpuid_base_hypervisor(const char *sig, u32 leaves)
{
	u32 base, eax, signature[3];

	for_each_possible_cpuid_base_hypervisor(base) {
		cpuid(base, &eax, &signature[0], &signature[1], &signature[2]);

		if (!sig_differs(sig, signature) &&
		    (leaves == 0 || ((eax - base) >= leaves)))
			return base;
	}

	return 0;
}

/* arch/x86/include/uapi/asm/kvm_para.h */
#define KVM_SIGNATURE "KVMKVMKVM\0\0\0"
#define KVM_CPUID_FEATURES	0x40000001
#define KVM_FEATURE_CLOCKSOURCE2        3
#define MSR_KVM_SYSTEM_TIME_NEW 0x4b564d01

/* arch/x86/kernel/kvm.c, the cpuid_level check and the cache cut */
static u32 __kvm_cpuid_base(void)
{
	if ((cpuid_ecx(1) >> X86_FEATURE_HYPERVISOR_BIT) & 1)	/* boot_cpu_has(X86_FEATURE_HYPERVISOR) */
		return cpuid_base_hypervisor(KVM_SIGNATURE, 0);

	return 0;
}

static unsigned int kvm_arch_para_features(void)
{
	return cpuid_eax(__kvm_cpuid_base() | KVM_CPUID_FEATURES);
}

/* include/linux/kvm_para.h */
static inline int kvm_para_has_feature(unsigned int feature)
{
	return !!(kvm_arch_para_features() & (1UL << feature));
}

/* arch/x86/include/asm/pvclock-abi.h */
struct pvclock_vcpu_time_info {
	u32   version;
	u32   pad0;
	u64   tsc_timestamp;
	u64   system_time;
	u32   tsc_to_system_mul;
	s8    tsc_shift;
	u8    flags;
	u8    pad[2];
} __attribute__((__packed__)); /* 32 bytes */

/* arch/x86/include/asm/msr.h, without the exception table */
static inline void __wrmsrq(u32 msr, u64 val)
{
	asm volatile("wrmsr"
		     : : "c" (msr), "a" ((u32)val), "d" ((u32)(val >> 32)) : "memory");
}

/* arch/x86/include/asm/div64.h, without the power-of-two branch */
#define do_div(n, base)						\
({								\
	unsigned long __upper, __low, __high, __mod, __base;	\
	__base = (base);					\
	{							\
		asm("" : "=a" (__low), "=d" (__high) : "A" (n));\
		__upper = __high;				\
		if (__high) {					\
			__upper = __high % (__base);		\
			__high = __high / (__base);		\
		}						\
		asm("divl %2" : "=a" (__low), "=d" (__mod)	\
			: "rm" (__base), "0" (__low), "1" (__upper));	\
		asm("" : "=A" (n) : "a" (__low), "d" (__high));	\
	}							\
	__mod;							\
})

/* arch/x86/kernel/pvclock.c */
static unsigned long pvclock_tsc_khz(struct pvclock_vcpu_time_info *src)
{
	u64 pv_tsc_khz = 1000000ULL << 32;

	do_div(pv_tsc_khz, src->tsc_to_system_mul);
	if (src->tsc_shift < 0)
		pv_tsc_khz <<= -src->tsc_shift;
	else
		pv_tsc_khz >>= src->tsc_shift;
	return pv_tsc_khz;
}

/* arch/x86/kernel/kvmclock.c: one page for one processor, never crossing a
 * page boundary (KVM's rule), lent only for the length of kvmclock_khz() */
static struct pvclock_vcpu_time_info hv_clock __attribute__((aligned(32)));
static int msr_kvm_system_time;

static void kvm_register_clock(void)
{
	u64 pa;

	pa = (unsigned long)&hv_clock | 0x01ULL;
	__wrmsrq(msr_kvm_system_time, pa);
}

static void kvmclock_disable(void)
{
	if (msr_kvm_system_time)
		__wrmsrq(msr_kvm_system_time, 0);
}

static unsigned long kvm_get_tsc_khz(void)
{
	return pvclock_tsc_khz(&hv_clock);
}

/* Ours: kvmclock_init's MSR choice, then ask once and give the page back.
 * KVM fills the page before the guest runs again, so it is ready when wrmsr
 * returns; an odd version means mid-update, and then we would rather say 0. */
unsigned long kvmclock_khz(void)
{
	unsigned long khz = 0;

	if (!__kvm_cpuid_base())	/* kvm_para_available() */
		return 0;
	if (kvm_para_has_feature(KVM_FEATURE_CLOCKSOURCE2))
		msr_kvm_system_time = MSR_KVM_SYSTEM_TIME_NEW;
	else
		return 0;

	kvm_register_clock();
	if (hv_clock.version && !(hv_clock.version & 1) && hv_clock.tsc_to_system_mul)
		khz = kvm_get_tsc_khz();
	kvmclock_disable();
	return khz;
}
