/* CMP.loader@2 link script.  One ELF, loaded by the machine at 1 MiB, entered at _start.
 * The .note.Xen section must land in a PT_NOTE segment: that is how the machine
 * finds the entry point (QEMU hw/i386/x86-common.c load_elfboot). */
ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

PHDRS
{
	text PT_LOAD FLAGS(5);
	data PT_LOAD FLAGS(6);
	note PT_NOTE FLAGS(4);
}

SECTIONS
{
	. = 0x100000;
	.text   : { *(.text .text.*) } :text
	.rodata : { *(.rodata .rodata.*) } :text
	.note.Xen : { KEEP(*(.note.Xen)) } :text :note
	. = ALIGN(4096);
	.data   : { *(.data .data.*) } :data
	.bss    : { *(.bss .bss.* COMMON) } :data
	/DISCARD/ : { *(.comment) *(.eh_frame) *(.note.gnu.*) *(.note.GNU-stack) }
}
