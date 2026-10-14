/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.loader@6: the way in when the processor is interrupted, and the way out
 * into a program.  Ours; nothing lifted.  Every entry leaves the same frame on
 * the stack (eight registers, the vector, an error code, then what the
 * processor pushed: eip, cs, eflags), so one path saves a program and one path
 * resumes one.  kernel_trap decides; it returns the stack to resume, and sets
 * next_cr3 to the address space that stack lives in.
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
	/* the tick (IRQ 0), the PIC's spurious IRQ 7, the APIC's spurious vector */
	TRAP 0x30
	TRAP 0x37
	TRAP 0xff

trap_common:
	pushal
	pushl	%esp
	call	kernel_trap
	addl	$4, %esp
	movl	next_cr3, %ecx
	movl	%ecx, %cr3
	movl	%eax, %esp
	popal
	addl	$8, %esp
	iret

/* task_resume(cr3, esp): switch to that address space and that stack, and
 * return into the program as if from an interrupt.  Never returns. */
	.globl	task_resume
task_resume:
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	movl	%ecx, %cr3
	movl	%eax, %esp
	popal
	addl	$8, %esp
	iret

	.section .note.GNU-stack, "", @progbits
