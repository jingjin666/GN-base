#include <chinos/config.h>

#include <k_stdio.h>
#include <k_stddef.h>
#include <k_assert.h>
#include <k_debug.h>

#include <elf.h>

#define ARCH_ELF_DEBUG
#ifdef ARCH_ELF_DEBUG
#define arch_elf_dbg kdbg
#define arch_elf_err kerr
#define arch_elf_warn kwarn
#define arch_elf_info kinfo
#else
#define arch_elf_dbg(fmt, ...)
#define arch_elf_err(fmt, ...)
#define arch_elf_warn(fmt, ...)
#define arch_elf_info(fmt, ...)
#endif

int elf_arch_check(Elf64_Ehdr *hdr)
{
	if (!hdr)
	{
	    arch_elf_err("ELF header null\n");
		return -1;
	}

	if (hdr->e_ident[EI_CLASS] != ELFCLASS64)
	{
	    arch_elf_err("Need 64-bit objects: e_ident[EI_CLASS] = 0x%02x\n", hdr->e_ident[EI_CLASS]);
		return -1;
	}

	if (hdr->e_ident[EI_DATA] != ELFDATA2LSB)
	{
	    arch_elf_err("ERROR: Wrong endian-ness: e_ident[EI_DATA] = 0x%02x\n", hdr->e_ident[EI_DATA]);
		return -1;
	}

	if (hdr->e_ident[EI_VERSION] != 1)
	{
	    arch_elf_err("ELF VERSION ERROR e_ident[EI_VERSION] = 0x%02x\n", hdr->e_ident[EI_VERSION]);
		return -1;
	}
    
    if (hdr->e_machine != EM_ARM64)
    {
        arch_elf_err("ERROR: Not for ARM: e_machine = 0x%04x\n", hdr->e_machine);
        return -1;
    }

	return 0;
}

