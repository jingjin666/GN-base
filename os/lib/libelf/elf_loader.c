#include <chinos/config.h>

#include <k_stdio.h>
#include <k_stdint.h>
#include <k_stddef.h>
#include <k_assert.h>
#include <k_debug.h>
#include <uapi/errors.h>
#include <elf.h>

#include "elf_loader.h"

#define ELF_DEBUG
#ifdef ELF_DEBUG
#define elf_dbg kdbg
#define elf_err kerr
#define elf_warn kwarn
#define elf_info kinfo
#else
#define elf_dbg(fmt, ...)
#define elf_err(fmt, ...)
#define elf_warn(fmt, ...)
#define elf_info(fmt, ...)
#endif

#include <init.h>
#include <gran.h>
static void *calloc(size_t n, size_t size)
{
    void *mem = gran_alloc(g_heap, size);
    k_memset(mem, 0, size);
    return mem;
}

static void dump_elf_header(Elf_Ehdr *hdr)
{
    elf_info("ELF Header:\n");
    elf_info("  e_ident:      0x%02x 0x%02x 0x%02x 0x%02x\n",
      hdr->e_ident[0], hdr->e_ident[1],
      hdr->e_ident[2], hdr->e_ident[3]);
    elf_info("  e_type:       0x%04x\n",  hdr->e_type);
    elf_info("  e_machine:    0x%04x\n",  hdr->e_machine);
    elf_info("  e_version:    0x%08x\n",  hdr->e_version);
    elf_info("  e_entry:      0x%08lx\n", (long)hdr->e_entry);
    elf_info("  e_phoff:      %d\n",    hdr->e_phoff);
    elf_info("  e_shoff:      %d\n",    hdr->e_shoff);
    elf_info("  e_flags:      0x%08x\n" , hdr->e_flags);
    elf_info("  e_ehsize:     %d\n",    hdr->e_ehsize);
    elf_info("  e_phentsize:  %d\n",    hdr->e_phentsize);
    elf_info("  e_phnum:      %d\n",    hdr->e_phnum);
    elf_info("  e_shentsize:  %d\n",    hdr->e_shentsize);
    elf_info("  e_shnum:      %d\n",    hdr->e_shnum);
    elf_info("  e_shstrndx:   %d\n",    hdr->e_shstrndx);
}

static void segment_queue_push(struct segment *seg, struct chin_elf *elf)
{
	list_add_tail(&seg->link_head, &elf->segs);
}

static void segment_stack_push(struct segment *seg, struct chin_elf *elf)
{
	list_add(&seg->link_head, &elf->segs);
}

static void add_segment(struct chin_elf *elf, size_t type, size_t offset, uintptr_t vaddr,
			size_t filesz, size_t memsz, size_t flags, size_t align)
{
	struct segment *seg = (struct segment *)calloc(1, sizeof(struct segment));
	if (!seg) {
		elf_err("calloc\n");
		return;
	}

	if (memsz < filesz) {
		elf_err("Memsz smaller than filesz\n");
		return;
	}

	seg->type = type;
	seg->offset = offset;
	seg->vaddr = vaddr;
	seg->filesz = filesz;
	seg->memsz = memsz;
	seg->flags = flags;
	seg->align = align;

    if (seg->flags & PF_W) {
        elf_dbg("segment writeable %p\n", seg->vaddr);
        seg->remapped_writeable = true;
    }

	elf_dbg("seg = %p, seg->type = %p seg->vaddr = %p seg->filesz = %p seg->memsz = %p\n", seg, seg->type, seg->vaddr, seg->filesz, seg->memsz);

	/* mmap从高地址向低地址增长，使用栈数据结构，否则使用队列数据结构 */
	segment_stack_push(seg, elf);
}

static void load_segments(struct chin_elf *elf)
{
	uint32_t n = 0;

	if (elf->is_64bit)
    {
		Elf64_Phdr *phdr = elf->phdr;
		/* 循环解析程序头 */
		for (n = 0; n < elf->e_phnum; n++)
        {
            elf_dbg("segment type = %p\n", phdr[n].p_type);
            switch (phdr[n].p_type)
            {
                case PT_NULL:
                    PANIC();
                    break;
                case PT_LOAD:
                case PT_DYNAMIC:
                    /* 把PT_LOAD和PT_DYNAMIC段添加到segments循环链表中 */
    				add_segment(elf, phdr[n].p_type, phdr[n].p_offset,
    						phdr[n].p_vaddr, phdr[n].p_filesz,
    						phdr[n].p_memsz, phdr[n].p_flags,
    						phdr[n].p_align);
                    break;
                case PT_INTERP:
                    /* 有PT_INTERP代表需要链接器 */
                    elf->need_interpreter = true;
                    break;
                case PT_NOTE:
                case PT_SHLIB:
                case PT_PHDR:
                case PT_TLS:
                case PT_NUM:
                case PT_LOOS:
                case PT_GNU_EH_FRAME:
                case PT_GNU_STACK:
                case PT_GNU_RELRO:
                case PT_GNU_PROPERTY:
                //case PT_LOSUNW:
                case PT_SUNWBSS: 
                case PT_SUNWSTACK:
                //case PT_HISUNW:
                case PT_HIOS:
                case PT_LOPROC:
                    // todo
                    break;
                default:
                    break;
            }
        }
	}
}

static int copy_segment(struct chin_elf *elf, struct segment *seg)
{
	int retval = OK;
	size_t size = seg->filesz;
	size_t offset = seg->offset;
	size_t mem_size = seg->memsz;
	size_t addr = seg->vaddr;

	elf_dbg("addr = %p, file_off = %p, file_size = %p, mem_size = %p\n", addr, offset, size, mem_size);

    if (mem_size < size) {
        elf_err("Segment size error\n");
        return -EINVAL;
    }

	/* 拷贝code/data */
	char *buff = elf->buffer + offset;
    if (seg->flags == (PF_R|PF_X)) {
        elf_dbg("copy code = %p, size = %p\n", addr, size);
    } else if(seg->flags == (PF_R|PF_W)) {
        elf_dbg("copy data = %p, size = %p\n", addr, size);
    } else {
        elf_err("Unknown segment flags\n");
        return -EINVAL;
    }
	k_memcpy((char *)addr, buff, size);

	/* 有BSS段需要清空 */
	if (mem_size > size) {
		unsigned long bss_size = mem_size - size;
        /* 清BSS */
        elf_dbg("clean bss = %p, size = %p\n", addr + size, bss_size);
        k_memset((void *)(addr + size), 0, bss_size);
	}

	return retval;
}

static void elf_map(uintptr_t vaddr, size_t size, uint32_t flags, uintptr_t *addr)
{
    extern struct addrspace user_addrspace;
    struct mem_region elf_region;

    *addr = (unsigned long)calloc(1, size);

    elf_region.pbase = vbase_to_pbase(*addr);
    elf_region.vbase = vaddr;
    elf_region.size  = size;

    elf_dbg("as_map %p %p %p\n", elf_region.pbase, elf_region.vbase, elf_region.size);
    as_map(&user_addrspace.pg_table, &elf_region, flags);
}

static void populate_segments(struct chin_elf *elf)
{
    struct segment *seg;
    uintptr_t addr;

    /* 主模块段映射，把相关段拷贝到运行地址空间 */
    list_for_each_entry(seg, &elf->segs, link_head)
    {
        if (seg->type == PT_LOAD)
        {
            if (seg->flags == (PF_R|PF_X)) {
                // 映射RE段
                elf_dbg("elf_mmap, vm = %p, size = %p\n", seg->vaddr, seg->memsz);

                uintptr_t align_addr = seg->vaddr / seg->align * seg->align;
				size_t align_size = (seg->vaddr % seg->align) + seg->memsz;

                elf->load_addr = seg->vaddr;

                elf_map(align_addr, align_to(align_size, seg->align), seg->flags, &seg->vaddr);
                copy_segment(elf, seg);
            } else if (seg->flags == (PF_R|PF_W)) {
                // 映射RW段
                elf_dbg("elf_mmap, vm = %p, size = %p\n", seg->vaddr, seg->memsz);

                uintptr_t align_addr = seg->vaddr / seg->align * seg->align;
				size_t align_size = (seg->vaddr % seg->align) + seg->memsz;

                elf_map(align_addr, align_to(align_size, seg->align), seg->flags, &seg->vaddr);
                copy_segment(elf, seg);
            } else {
                // todo
                PANIC();
            }
        }
    }
}

int elf_initialize(struct chin_elf *elf)
{
    int ret = OK;
    Elf_Ehdr elf_header;

    assert(elf->buffer && elf->size);

    elf_dbg("elf->buffer = %p, elf->size = %p\n", elf->buffer, elf->size);

#ifdef CONFIG_ARCH_64
    elf_header = *(Elf64_Ehdr *)(elf->buffer);
    ret = elf64_header_check(&elf_header);
    elf->is_64bit = true;
#else
    elf_header = *(Elf32_Ehdr *)(elf->buffer);
    ret = elf32_header_check(&elf_header);
    elf->is_64bit = false;
#endif
    if(ret < 0) {
		elf_err("failed to check elf header\n");
		return -ENOEXEC;
	}

	ret = elf_arch_check(&elf_header);
	if(ret < 0) {
		elf_err("failed to check elf arch header\n");
		return -ENOEXEC;
	}

    dump_elf_header(&elf_header);
    
    elf->shdr = elf->buffer + elf_header.e_shoff;
    elf->phdr = elf->buffer + elf_header.e_phoff;
    elf_dbg("elf->shdr = %p, elf->phdr = %p\n", elf->shdr, elf->phdr);

    elf->e_type  = elf_header.e_type;
    elf->e_entry = elf_header.e_entry;
    elf->e_phoff = elf_header.e_phoff;
    elf->e_shoff = elf_header.e_shoff;
    elf->e_phnum = elf_header.e_phnum;
    elf->e_shnum = elf_header.e_shnum;
    elf->e_phentsize = elf_header.e_phentsize;
    elf->e_shentsize = elf_header.e_shentsize;

    elf_dbg("elf->entry = 0x%016x\n", elf->e_entry);

    load_segments(elf);

    populate_segments(elf);

    return ret;
}