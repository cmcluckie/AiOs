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
