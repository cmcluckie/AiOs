/* SPDX-License-Identifier: GPL-2.0-only
 *
 * CMP.loader@2 entry.  Unchanged from @1.  The first instruction that is ours.  Ours; nothing lifted.
 *
 * The machine's firmware (qboot, inside CMP.hypervisor.qemu) hands over under
 * the x86/HVM direct boot ABI ("PVH"), a public interface: Xen docs/misc/pvh.pandoc
 * states the register and segment contract; XEN_ELFNOTE_PHYS32_ENTRY (18) is from
 * Xen's public elfnote.h, MIT.  The descriptor values are the Intel SDM's.
 *
 * What the ABI gives us: 32-bit protected mode, paging off, cr4 clear, flat
 * code and data segments with unspecified selectors, interrupts off, no stack,
 * no IDT, ebx = physical address of hvm_start_info.
 *
 * What we therefore must do before a line of C, none of it asked for by any
 * petition (findings 0002): load our own GDT so the selectors are known, reload
 * the segment registers, give ourselves a stack.  Stay in 32-bit flat mode with
 * paging off: one program, one address space, nothing to translate.
 */

	.section .note.Xen, "a"
	.balign 4
	.long	4			/* namesz */
	.long	4			/* descsz */
	.long	18			/* XEN_ELFNOTE_PHYS32_ENTRY */
	.asciz	"Xen"
	.long	_start

	.text
	.code32
	.globl	_start
_start:
	cld
	lgdt	gdt_desc
	movl	$0x10, %eax		/* data32 */
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %ss
	ljmp	$0x08, $1f		/* code32 */
1:
	movl	$stack_top, %esp
	pushl	%ebx			/* struct hvm_start_info * */
	call	loader_main
	cli
	hlt
	jmp	1b

	.section .rodata
	.balign 8
gdt:
	.quad	0x0000000000000000	/* null */
	.quad	0x00cf9a000000ffff	/* 0x08: code, base 0, limit 4G, 32-bit */
	.quad	0x00cf92000000ffff	/* 0x10: data, base 0, limit 4G, 32-bit */
gdt_end:
gdt_desc:
	.word	gdt_end - gdt - 1
	.long	gdt

	.bss
	.balign 16
stack:
	.skip	4096
stack_top:

	.section .note.GNU-stack, "", @progbits
