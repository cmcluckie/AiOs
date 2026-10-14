/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.irq.pic@1: the two 8259A interrupt controllers the IBM PC/AT had, set up
 * the way Linux sets them up, and the processor's local APIC told to let them
 * through ("virtual wire" mode), the way Linux does when the firmware has not.
 * Only the master's lines are ever unmasked; the slave is initialised and left
 * masked.  Exposes pic_init, pic_unmask, pic_eoi.  Requires CAP.dev.pic.
 *
 * Lifted from Linux, commit a5ebb76233b79db01e82e061173cb73c6d2b5c6b (2026-10-10).
 * The machine-readable record is origin.toml beside this file; jig/upstream reads it.
 *   arch/x86/kernel/i8259.c            unmask_8259A_irq 79-91 (master half),
 *                                      the specific EOI in mask_and_ack_8259A 190,
 *                                      init_8259A 349-399
 *   arch/x86/include/asm/i8259.h       the ports 15-16 and 20-21, PIC_CASCADE_IR and
 *                                      the ICW4 defaults 26-28
 *   arch/x86/include/asm/irq_vectors.h FIRST_EXTERNAL_VECTOR 36, ISA_IRQ_VECTOR 44,
 *                                      SPURIOUS_APIC_VECTOR 54
 *   arch/x86/kernel/apic/apic.c        init_bsp_APIC: the SPIV enable 1335-1337,
 *                                      1346-1348 and the virtual wire 1353-1354,
 *                                      1358-1359
 *   arch/x86/include/asm/apicdef.h     APIC_DEFAULT_PHYS_BASE 15, APIC_SPIV 60, 62-63,
 *                                      APIC_DM_NMI 95, APIC_DM_EXTINT 98,
 *                                      APIC_VECTOR_MASK 99, APIC_LVT0 106,
 *                                      APIC_LVT_MASKED 110, APIC_LVT1 121
 *   arch/x86/include/asm/shared/io.h   BUILDIO 7-20: the inb and outb instructions
 *
 * Cut, and why:
 *   i8259A_lock, cached_irq_mask      one processor, interrupts off whenever these run
 *   as a lock and a cache             (the mask is kept as two plain bytes)
 *   auto-EOI, the irq_chip plumbing   normal EOI only, called by the one handler
 *   mask_and_ack's mask-then-EOI      the handler is short and runs with interrupts
 *                                     off; a plain specific EOI is enough
 *   spurious IRQ detection            the kernel gives vector 0x37 a handler that
 *   (i8259A_irq_real)                 does nothing, which is what a spurious IRQ 7 needs
 *   udelay(100) after init            no microsecond clock here (findings 0011); QEMU's
 *                                     8259 needs no settling time, a real one might
 *   outb_pic's slow-down               the same reason
 *   clear_local_APIC, the SMP and     one processor that has an APIC; the firmware left
 *   feature checks, the P4 focus bit  nothing in it worth clearing (findings 0011)
 *   the slave's lines                 nothing on them has a petition
 */

#include "pic.h"

/* arch/x86/include/asm/shared/io.h */
static inline unsigned char inb(unsigned short port)
{
	unsigned char v;
	__asm__ volatile("inb %w1, %b0" : "=a"(v) : "Nd"(port));
	return v;
}

static inline void outb(unsigned char v, unsigned short port)
{
	__asm__ volatile("outb %b0, %w1" : : "a"(v), "Nd"(port));
}
#define outb_pic outb

/* arch/x86/include/asm/i8259.h */
#define PIC_MASTER_CMD		0x20
#define PIC_MASTER_IMR		0x21
#define PIC_SLAVE_CMD		0xa0
#define PIC_SLAVE_IMR		0xa1
#define PIC_CASCADE_IR		2
#define MASTER_ICW4_DEFAULT	0x01
#define SLAVE_ICW4_DEFAULT	0x01

/* arch/x86/include/asm/irq_vectors.h */
#define FIRST_EXTERNAL_VECTOR		0x20
#define ISA_IRQ_VECTOR(irq)		(((FIRST_EXTERNAL_VECTOR + 16) & ~15) + irq)
#define SPURIOUS_APIC_VECTOR		0xff
_Static_assert(ISA_IRQ_VECTOR(0) == PIC_VECTOR(0), "pic.h agrees with Linux");
_Static_assert(SPURIOUS_APIC_VECTOR == PIC_SPURIOUS_APIC_VECTOR, "pic.h agrees with Linux");

/* arch/x86/include/asm/apicdef.h */
#define	APIC_DEFAULT_PHYS_BASE		0xfee00000
#define	APIC_SPIV	0xF0
#define		APIC_SPIV_FOCUS_DISABLED	(1 << 9)
#define		APIC_SPIV_APIC_ENABLED		(1 << 8)
#define		APIC_DM_NMI		0x00400
#define		APIC_DM_EXTINT		0x00700
#define		APIC_VECTOR_MASK	0x000FF
#define	APIC_LVT0	0x350
#define		APIC_LVT_MASKED			(1 << 16)
#define	APIC_LVT1	0x360

/* Glue: the local APIC's registers, memory-mapped where every PC puts them. */
static unsigned int apic_read(unsigned int reg)
{
	return *(volatile unsigned int *)(APIC_DEFAULT_PHYS_BASE + reg);
}

static void apic_write(unsigned int reg, unsigned int v)
{
	*(volatile unsigned int *)(APIC_DEFAULT_PHYS_BASE + reg) = v;
}

static unsigned char cached_master_mask = 0xff, cached_slave_mask = 0xff;

void pic_init(void)
{
	unsigned int value;

	/* i8259.c init_8259A, normal EOI */
	outb(0xff, PIC_MASTER_IMR);	/* mask all of 8259A-1 */

	outb_pic(0x11, PIC_MASTER_CMD);	/* ICW1: select 8259A-1 init */

	/* ICW2: 8259A-1 IR0-7 mapped to ISA_IRQ_VECTOR(0) */
	outb_pic(ISA_IRQ_VECTOR(0), PIC_MASTER_IMR);

	/* 8259A-1 (the master) has a slave on IR2 */
	outb_pic(1U << PIC_CASCADE_IR, PIC_MASTER_IMR);

	outb_pic(MASTER_ICW4_DEFAULT, PIC_MASTER_IMR);

	outb_pic(0x11, PIC_SLAVE_CMD);	/* ICW1: select 8259A-2 init */

	/* ICW2: 8259A-2 IR0-7 mapped to ISA_IRQ_VECTOR(8) */
	outb_pic(ISA_IRQ_VECTOR(8), PIC_SLAVE_IMR);
	/* 8259A-2 is a slave on master's IR2 */
	outb_pic(PIC_CASCADE_IR, PIC_SLAVE_IMR);
	outb_pic(SLAVE_ICW4_DEFAULT, PIC_SLAVE_IMR);

	outb(cached_master_mask, PIC_MASTER_IMR); /* restore master IRQ mask */
	outb(cached_slave_mask, PIC_SLAVE_IMR);	  /* restore slave IRQ mask */

	/* apic/apic.c init_bsp_APIC: enable the APIC, then the virtual wire */
	value = apic_read(APIC_SPIV);
	value &= ~APIC_VECTOR_MASK;
	value |= APIC_SPIV_APIC_ENABLED;
	value |= APIC_SPIV_FOCUS_DISABLED;
	value |= SPURIOUS_APIC_VECTOR;
	apic_write(APIC_SPIV, value);

	apic_write(APIC_LVT0, APIC_DM_EXTINT);
	value = APIC_DM_NMI;
	value |= APIC_LVT_MASKED;
	apic_write(APIC_LVT1, value);
}

/* i8259.c unmask_8259A_irq, the master's half */
void pic_unmask(unsigned int irq)
{
	unsigned int mask = ~(1 << irq);

	cached_master_mask &= mask;
	outb(cached_master_mask, PIC_MASTER_IMR);
}

/* i8259.c mask_and_ack_8259A: the master's specific EOI */
void pic_eoi(unsigned int irq)
{
	outb(0x60+irq, PIC_MASTER_CMD);	/* 'Specific EOI to master */
}
