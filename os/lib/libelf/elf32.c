#include <chinos/config.h>

#ifdef CONFIG_ARCH_32
#include <k_stdio.h>
#include <k_stdint.h>
#include <k_stddef.h>
#include <k_assert.h>
#include <k_debug.h>

#include <elf.h>

#define ELF32_DEBUG
#ifdef ELF32_DEBUG
#define elf32_dbg kdbg
#define elf32_err kerr
#define elf32_warn kwarn
#define elf32_info kinfo
#else
#define elf32_dbg(fmt, ...)
#define elf32_err(fmt, ...)
#define elf32_warn(fmt, ...)
#define elf32_info(fmt, ...)
#endif
    
int elf32_header_check(Elf32_Ehdr *hdr)
{
    if (!hdr)
    {
        elf32_err("ELF header null\n");
        return -1;
    }

	if (hdr->e_ident[0] != 0x7F || hdr->e_ident[1] != 'E' ||
		hdr->e_ident[2] != 'L' || hdr->e_ident[3] != 'F')
	{
	    elf32_err("Not ELF file\n");
		return -1;
	}

    if (hdr->e_ident[EI_CLASS] != ELFCLASS32)
    {
        elf32_err("Need 32-bit objects: e_ident[EI_CLASS] = 0x%02x\n", hdr->e_ident[EI_CLASS]);
        return -1;
    }

	if (hdr->e_type != ET_DYN && hdr->e_type != ET_REL && hdr->e_type != ET_EXEC)
	{
	    elf32_err("Unsupported ELF type = 0x%04x\n", hdr->e_type);
		return -1;
	}

    if (hdr->e_phentsize != sizeof(Elf32_Phdr)) {
        elf32_err("hdr->e_phentsize = %d %d\n", hdr->e_phentsize, sizeof(Elf32_Phdr));
        return -1;
    }

    if (hdr->e_phnum < 1 ||
        hdr->e_phnum > 65536U / sizeof(Elf32_Phdr)) {
        elf32_err("hdr->e_phnum = %d\n", hdr->e_phnum);
        return -1;
    }

    if(hdr->e_shentsize != sizeof(Elf32_Shdr)) {
        elf32_err("hdr->e_shentsize = %d %p\n", hdr->e_shentsize, sizeof(Elf32_Shdr));
        return -1;
    }
    
    if (hdr->e_shnum < 1) {
        elf32_err("hdr->e_shnum = %d\n", hdr->e_shnum);
        return -1;
    }

    if (hdr->e_shstrndx >= hdr->e_shnum) {
        elf32_err("hdr->e_shstrndx = %d %d\n", hdr->e_shstrndx, hdr->e_shnum);
        return -1;
    }

    return 0;
}
#endif
