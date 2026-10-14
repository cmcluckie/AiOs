/* PART.loader@1: the program format.  A flat binary linked at PROGRAM_BASE with
 * its entry at byte 0, called with the door (door.h).  Every program on the
 * ladder links with this file; it belongs to the loader, not the program. */
ENTRY(_start)
OUTPUT_FORMAT(binary)

SECTIONS
{
	. = 0x400000;
	.text   : { KEEP(*(.text.start)) *(.text .text.*) }
	.rodata : { *(.rodata .rodata.*) }
	.data   : { *(.data .data.*) }
	.bss    : { *(.bss .bss.* COMMON) }
	/DISCARD/ : { *(.comment) *(.eh_frame) *(.note.*) }
}
