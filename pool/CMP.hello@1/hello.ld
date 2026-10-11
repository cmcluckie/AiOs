/* CMP.hello@1 link script.  A flat binary, linked at the address the loader runs
 * programs from (CMP.loader@1 PROGRAM_BASE), entry at byte 0. */
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
