ENTRY(_start);

SECTIONS
{
    . = 0xffffff8000000000;
    kernel_start = .;
	boot_physic_start = .;
    .boot.physic : AT(ADDR(.boot.physic) - 0xffffff8000000000 + 0x40000000)
    {
        *(.entry)
        *(.boot.physic)
        . = ALIGN(4096);
        . += 4096;
        boot_stack = .;
    }

    . = ALIGN(4096);
    boot_physic_end = .;
    .boot : AT(ADDR(.boot) - 0xffffff8000000000 + 0x40000000)
    {
        *(.boot);
    }
    .boot.data : AT(ADDR(.boot.data) - 0xffffff8000000000 + 0x40000000)
    {
        *(.boot.data);
    }

    . = ALIGN(4096);
    boot_end = .;
    .text : AT(ADDR(.text) - 0xffffff8000000000 + 0x40000000)
    {
        *(.vector);
        *(.text*)
    }

    .rodata : AT(ADDR(.rodata) - 0xffffff8000000000 + 0x40000000)
    {
        *(.rodata*)
    }

	. = ALIGN(4096);
    .data : AT(ADDR(.data) - 0xffffff8000000000 + 0x40000000)
    {
        *(.data*)
    }

	. = ALIGN(4096);
    .stack : AT(ADDR(.stack) - 0xffffff8000000000 + 0x40000000)
    {
        *(.stack*)
    }

	. = ALIGN(4096);
    .bss : AT(ADDR(.bss) - 0xffffff8000000000 + 0x40000000)
    {
        *(.bss*)
    }

	. = ALIGN(4096);
    kernel_end = .;

    /DISCARD/ :
    {
        *(.note.gnu.build-id)
        *(.comment)
    }
}
