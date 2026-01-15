/*
 * Linker script for ValcOS userspace programs
 */

ENTRY(_start)

SECTIONS
{
    . = 0x400000;  /* User programs start at 4MB */

    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
        *(COMMON)
    }
}
