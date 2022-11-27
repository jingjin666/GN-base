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
    kernel_code_start = .;
    .text : AT(ADDR(.text) - 0xffffff8000000000 + 0x40000000)
    {
        *(.vector);
        *(.text*)
    }

    . = ALIGN(4096);
    kernel_code_end = .;
    kernel_rodata_start = .;
    .rodata : AT(ADDR(.rodata) - 0xffffff8000000000 + 0x40000000)
    {
        *(.rodata*)
    }

    . = ALIGN(4096);
    kernel_rodata_end = .;
    kernel_data_start = .;
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
    kernel_data_end = .;
    kernel_bss_start = .;
    .bss : AT(ADDR(.bss) - 0xffffff8000000000 + 0x40000000)
    {
        *(.bss*)
    }

    . = ALIGN(4096);
    kernel_bss_end = .;
    kernel_end = .;

    /DISCARD/ :
    {
        *(.note.gnu.build-id)
        *(.comment)
    }
}

SECTIONS
{
    . = ALIGN(4096);
    user_start = .;
    .user :
    {
        *(.user*)
    }
    . = ALIGN(4096);
    user_end = .;
}