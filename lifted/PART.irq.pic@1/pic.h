/* PART.irq.pic@1: the PC's 8259A interrupt controllers, wired through the processor's local APIC.  See pic.c for provenance. */
#ifndef AIOS_PIC_H
#define AIOS_PIC_H

#define PIC_VECTOR(irq)	(0x30 + (irq))	/* where IRQ n arrives in the IDT */
#define PIC_SPURIOUS_APIC_VECTOR 0xff

void pic_init(void);			/* every line masked, IRQs at 0x30-0x3f, the APIC passing them through */
void pic_unmask(unsigned int irq);	/* let one master line through */
void pic_eoi(unsigned int irq);		/* done with this one */

#endif
