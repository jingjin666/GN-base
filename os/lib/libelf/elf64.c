#include <chinos/config.h>

#ifdef CONFIG_ARCH_64    
#include <k_stdio.h>
#include <k_stdint.h>
#include <k_stddef.h>
#include <k_assert.h>
#include <k_debug.h>

#include <elf.h>

#define ELF64_DEBUG
#ifdef ELF64_DEBUG
#define elf64_dbg kdbg
#define elf64_err kerr
#define elf64_warn kwarn
#define elf64_info kinfo
#else
#define elf64_dbg(fmt, ...)
#define elf64_err(fmt, ...)
#define elf64_warn(fmt, ...)
#define elf64_info(fmt, ...)
#endif
    
int elf64_header_check(Elf64_Ehdr *hdr)
{
    if (!hdr)
    {
        elf64_err("ELF header null\n");
        return -1;
    }

	if (hdr->e_ident[0] != 0x7F || hdr->e_ident[1] != 'E' ||
		hdr->e_ident[2] != 'L' || hdr->e_ident[3] != 'F')
	{
	    elf64_err("Not ELF file\n");
		return -1;
	}

    if (hdr->e_ident[EI_CLASS] != ELFCLASS64)
    {
        elf64_err("Need 64-bit objects: e_ident[EI_CLASS] = 0x%02x\n", hdr->e_ident[EI_CLASS]);
        return -1;
    }

	if (hdr->e_type != ET_DYN && hdr->e_type != ET_REL && hdr->e_type != ET_EXEC)
	{
	    elf64_err("ERROR: ELF type = 0x%04x\n", hdr->e_type);
		return -1;
	}

    if (hdr->e_phentsize != sizeof(Elf64_Phdr)) {
        elf64_err("hdr->e_phentsize = %d %d\n", hdr->e_phentsize, sizeof(Elf64_Phdr));
        return -1;
    }

    if (hdr->e_phnum < 1 ||
        hdr->e_phnum > 65536U / sizeof(Elf64_Phdr)) {
        elf64_err("hdr->e_phnum = %d\n", hdr->e_phnum);
        return -1;
    }

    if(hdr->e_shentsize != sizeof(Elf64_Shdr)) {
        elf64_err("hdr->e_shentsize = %d %p\n", hdr->e_shentsize, sizeof(Elf64_Shdr));
        return -1;
    }
    
    if (hdr->e_shnum < 1) {
        elf64_err("hdr->e_shnum = %d\n", hdr->e_shnum);
        return -1;
    }

    if (hdr->e_shstrndx >= hdr->e_shnum) {
        elf64_err("hdr->e_shstrndx = %d %d\n", hdr->e_shstrndx, hdr->e_shnum);
        return -1;
    }

    return 0;
}
#endif
