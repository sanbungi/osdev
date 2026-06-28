/* linker.ld */
OUTPUT_FORMAT("elf32-i386")
ENTRY(stage2_entry)

SECTIONS
{
    . = 0x8000;

    .text :
    {
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
    }

    .bss :
    {
        *(.bss*)
        *(COMMON)
    }
}
