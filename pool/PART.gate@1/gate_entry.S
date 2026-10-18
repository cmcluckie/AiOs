/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.gate@1: the way in when the processor is trapped or faults, and the way
 * out into ring 3.  Ours; nothing lifted.  Every entry leaves the same frame on
 * the stack so one path saves and one path resumes: ds, es, eight registers
 * (popal order), the vector and an error code first, and what the processor
 * pushed (eip, cs, eflags, and esp and ss when it came from ring 3).  The
 * kernel's data segments are reloaded here, because a program may leave
 * anything in its own.
 */

	.text
	.code32

.macro TRAP vec, errcode=0
	.globl	trap\vec
trap\vec:
	.if \errcode == 0
	pushl	$0
	.endif
	pushl	$\vec
	jmp	trap_common
.endm

	/* the processor's own exceptions; 8, 10-14, 17, 21, 29, 30 push an error code */
	TRAP 0
	TRAP 1
	TRAP 2
	TRAP 3
	TRAP 4
	TRAP 5
	TRAP 6
	TRAP 7
	TRAP 8, 1
	TRAP 9
	TRAP 10, 1
	TRAP 11, 1
	TRAP 12, 1
	TRAP 13, 1
	TRAP 14, 1
	TRAP 15
	TRAP 16
	TRAP 17, 1
	TRAP 18
	TRAP 19
	TRAP 20
	TRAP 21, 1
	TRAP 22
	TRAP 23
	TRAP 24
	TRAP 25
	TRAP 26
	TRAP 27
	TRAP 28
	TRAP 29, 1
	TRAP 30, 1
	TRAP 31
	TRAP 0x80
	/* hardware interrupts, when a sharing part asks for them (gate_schedule): the
	 * 8259's sixteen from 0x30, and the APIC's spurious vector */
	.irp v, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0xff
	TRAP \v
	.endr

	.data
	.globl	trap_irq_table
trap_irq_table:
	.long	trap0x30, trap0x31, trap0x32, trap0x33, trap0x34, trap0x35, trap0x36, trap0x37
	.long	trap0x38, trap0x39, trap0x3a, trap0x3b, trap0x3c, trap0x3d, trap0x3e, trap0x3f
	.long	trap0xff
	.text

trap_common:
	pushl	%ds
	pushl	%es
	pushal
	movl	$0x10, %eax
	movl	%eax, %ds
	movl	%eax, %es
	cld
	pushl	%esp
	call	gate_trap
	addl	$4, %esp
	movl	%eax, %esp
	popal
	popl	%es
	popl	%ds
	addl	$8, %esp
	iret

/* gate_resume_frame(cr3, frame): switch to that address space, and return into
 * the program whose registers were saved at FRAME as if from a trap.  Never returns. */
	.globl	gate_resume_frame
gate_resume_frame:
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	movl	%ecx, %cr3
	movl	%eax, %esp
	popal
	popl	%es
	popl	%ds
	addl	$8, %esp
	iret

/* gate_enter(eip, esp, eflags): drop to ring 3 at eip with that stack and those
 * flags, every register clear.  Never returns. */
	.globl	gate_enter
gate_enter:
	movl	4(%esp), %eax
	movl	8(%esp), %ecx
	movl	12(%esp), %edx
	movl	$0x23, %ebx
	movl	%ebx, %ds
	movl	%ebx, %es
	movl	%ebx, %fs
	movl	%ebx, %gs
	pushl	$0x23			/* ss */
	pushl	%ecx			/* esp */
	pushl	%edx			/* eflags */
	pushl	$0x1b			/* cs */
	pushl	%eax			/* eip */
	xorl	%eax, %eax
	xorl	%ebx, %ebx
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	xorl	%esi, %esi
	xorl	%edi, %edi
	xorl	%ebp, %ebp
	iret

	.section .note.GNU-stack, "", @progbits
