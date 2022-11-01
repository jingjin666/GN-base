ENTRY(_start);

SECTIONS
{
    . = 0x40000000;

    .boot.physic : AT(ADDR(.boot.physic))
    {
        *(.entry)
        *(.boot.physic)
        . = ALIGN(4096);
        . += 4096;
        boot_stack = .;
    }

    . = ALIGN(4096);
    .boot : AT(ADDR(.boot))
    {
        *(.boot);
    }
    .boot.data : AT(ADDR(.boot.data))
    {
        *(.boot.data);
    }
    
    . = ALIGN(4096);
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

    /DISCARD/ :
    {
        *(.note.gnu.build-id)
        *(.comment)
    }
}
