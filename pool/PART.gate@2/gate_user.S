/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.gate@1: the only code and data of the kernel a program can see, one page
 * (the linker script pads it to 4096 bytes and aligns it).  The system call
 * table a program is handed, and for each of its seven entries a stub that asks
 * the kernel with int 0x80.  Same table, same offsets as the loader has
 * always handed over (door.h); what is behind each entry is now the gate.
 * 32-bit cdecl: the stub keeps ebx for its caller.  Ours.
 *
 * TECH-DEBT 1: alloc (1000), ms (1001), load (1002) and save (1003) use
 * AiOs-only numbers (findings 0021 #5).
 */

	.section .gate_user, "ax", @progbits
	.code32

	.globl	gate_door
gate_door:
	.long	d_write, d_exit, d_alloc, d_read, d_ms, d_load, d_save

d_write:
	pushl	%ebx
	movl	$4, %eax
	movl	$1, %ebx
	movl	8(%esp), %ecx
	movl	12(%esp), %edx
	int	$0x80
	popl	%ebx
	ret

d_exit:
	movl	4(%esp), %ebx
	movl	$1, %eax
	int	$0x80
1:	jmp	1b

d_alloc:
	pushl	%ebx
	movl	$1000, %eax
	movl	8(%esp), %ebx
	int	$0x80
	popl	%ebx
	ret

d_read:
	pushl	%ebx
	movl	$3, %eax
	xorl	%ebx, %ebx
	movl	8(%esp), %ecx
	movl	12(%esp), %edx
	int	$0x80
	popl	%ebx
	ret

d_ms:
	movl	$1001, %eax
	int	$0x80
	ret

d_load:
	pushl	%ebx
	movl	$1002, %eax
	movl	8(%esp), %ebx
	movl	12(%esp), %ecx
	movl	16(%esp), %edx
	int	$0x80
	popl	%ebx
	ret

d_save:
	pushl	%ebx
	movl	$1003, %eax
	movl	8(%esp), %ebx
	movl	12(%esp), %ecx
	movl	16(%esp), %edx
	int	$0x80
	popl	%ebx
	ret

/* where _start returns to: a program that returns has exited */
	.globl	gate_return
gate_return:
	xorl	%ebx, %ebx
	movl	$1, %eax
	int	$0x80
1:	jmp	1b

	.section .note.GNU-stack, "", @progbits
