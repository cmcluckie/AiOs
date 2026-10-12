/* CMP.loader@4: the program format.  A flat binary linked at the address the loader
 * runs programs from (PROGRAM_BASE), entry at byte 0, called with the door.
 * Zero-initialised data (.bss) is written into the file as zeros, because a flat
 * binary has no other way to say how much memory it needs and the loader's heap
 * starts where the file ends (findings 0005 #1).  Every program on the ladder
 * links with this file; it belongs to the loader, not the program. */
ENTRY(_start)
OUTPUT_FORMAT(binary)

SECTIONS
{
	. = 0x400000;
	.text   : { KEEP(*(.text.start)) *(.text .text.*) }
	.rodata : { *(.rodata .rodata.*) }
	.data   : { *(.data .data.*) *(.bss .bss.* COMMON) . = ALIGN(16); }
	/DISCARD/ : { *(.comment) *(.eh_frame) *(.note.*) }
}
