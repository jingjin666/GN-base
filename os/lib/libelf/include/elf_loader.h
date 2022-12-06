#ifndef __ELF_LOADER_H
#define __ELF_LOADER_H

#include <chinos/config.h>

#include <k_types.h>
#include <k_stdbool.h>
#include <uapi/list.h>
#include <task.h>

struct segment
{
	struct list_head link_head;
	size_t      type;
	size_t      offset;
	uintptr_t   vaddr;
	size_t      filesz;
	size_t      memsz;
	uint32_t    flags;
	size_t      align;
	bool        remapped_writeable;
};

struct chin_elf
{
    struct list_head link_head;

	bool need_interpreter;
	bool is_64bit;

    uint8_t *buffer;
    size_t size;

	uint16_t  e_type;
	uintptr_t e_entry;
	uintptr_t e_phoff;
	uintptr_t e_shoff;
	uint32_t  e_phnum;
	uint32_t  e_shnum;
	uint32_t  e_phentsize;
	uint32_t  e_shentsize;

    void *shdr;
	void *phdr;

    struct list_head segs;
};

int elf_initialize(struct tcb *task, struct chin_elf *elf);

#endif
