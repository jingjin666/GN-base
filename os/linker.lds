ENTRY(_start);

SECTIONS
{
    . = 0x40000000;

    .boot : AT(ADDR(.boot))
    {
        *(.boot)
    }

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
    }

    . = ALIGN(4096);
    . += 4096;
    boot_stack = .;

    /DISCARD/ :
    {
        *(.note.gnu.build-id)
        *(.comment)
    }
}
