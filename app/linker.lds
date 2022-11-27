ENTRY(_start);

SECTIONS
{
    . = 0x0000000000000000;
    app_start = .;
    app_code_start = .;
    .text :
    {
        *(.text*)
    }
    app_code_end = .;
    app_rodata_start = .;
    .rodata :
    {
        *(.rodata*)
    }

    . = ALIGN(4096);
    app_rodata_end = .;
    app_data_start = .;
    .data :
    {
        *(.data*)
    }
    app_data_end = .;
    app_bss_start = .;
    .bss :
    {
        *(.bss*)
    }

    . = ALIGN(4096);
    app_bss_end = .;
    app_end = .;

    /DISCARD/ :
    {
        *(.note.gnu.build-id)
        *(.comment)
    }
}