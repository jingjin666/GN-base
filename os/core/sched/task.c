#include <chinos/config.h>

#include <kernel.h>
#include <k_stddef.h>
#include <k_stdbool.h>
#include <k_string.h>
#include <k_limits.h>
#include <k_assert.h>
#include <k_stdlib.h>
#include <k_debug.h>
#include <scheduler.h>
#include <uapi/errors.h>
#include <addrspace.h>
#include <context.h>
#include <bitmap.h>
#include <instructionset.h>
#include <tlb.h>
#include <uapi/util.h>
#include <registerset.h>
#ifdef CONFIG_HYPERVISOR_SUPPORT
#include <mmu-hyper.h>
#include <vcpu.h>
#else
#include <mmu.h>
#endif

#include "task.h"

#define KERNEL_SCHED_TASK_DEBUG
#ifdef KERNEL_SCHED_TASK_DEBUG
#define task_dbg kdbg
#define task_err kerr
#define task_warn kwarn
#define task_info kinfo
#else
#define task_dbg(fmt, ...)
#define task_err(fmt, ...)
#define task_warn(fmt, ...)
#define task_info(fmt, ...)
#endif

#define INVALID_TASK_ID          (pid_t)(-1)
#define MAX_TASKS_MASK           (CONFIG_MAX_TASKS-1)

static bool b_tlb_flush;
static u32 asid_bits;
static volatile asid_t asid_generation;
static asid_t *asid_bitmap;

#define ASID_MASK		    (GENMASK(asid_bits - 1, 0))
#define ASID_FIRST_VERSION	BIT(asid_bits)

#define NUM_USER_ASIDS		ASID_FIRST_VERSION

static int get_cpu_asid_bits(void)
{
    u64 mmfr0, asid;
    MRS("ID_AA64MMFR0_EL1", mmfr0);

    asid = bitfield_get(mmfr0, ID_AA64MMFR0_ASID_SHIFT, 4);
    task_dbg("ID_AA64MMFR0_EL1.ASIDBITS = 0x%lx\n", asid);

    switch(asid)
    {
        case 0:
            return 8;
        case 2:
            return 16;
        default:
            PANIC();
    }

    return -1;
}

static asid_t check_and_switch_context(struct tcb *task)
{
    static u32 cur_idx = 1;
    asid_t asid = task->asid;
    asid_t generation = asid_generation;

    if (task->type == TASK_TYPE_KERNEL)
        return 0;

    // 当前任务的asid的版本和全局asid版本一致，不需要重新分配asid，直接返回
    if ((asid & ~ASID_MASK) == generation)
        return asid;

    // 检查在当前位图中是否存在可用的asid
    if (asid) {
        u64 newasid = generation | (asid & ASID_MASK);
        // 如果存在未使用的asid则bitmap_set然后直接返回，否则继续寻找下一个空位进行重新分配
        if (!test_and_set_bit((asid & ASID_MASK), asid_bitmap))
			return newasid;
    }

    // 分配新的asid
    asid = find_next_zero_bit(asid_bitmap, NUM_USER_ASIDS, cur_idx);
    if (asid >= NUM_USER_ASIDS) {
        u32 idx = generation >> asid_bits;
        idx++;
        asid_generation = idx << asid_bits;
        generation = asid_generation;

        // set flush tlb flag
        b_tlb_flush = true;

        // flush bitmap
        bitmap_clear(asid_bitmap, 0, NUM_USER_ASIDS);

        // get new asid
        asid = find_next_zero_bit(asid_bitmap, NUM_USER_ASIDS, 1);
    }

    bitmap_set(asid_bitmap, asid, 1);
    task->asid = asid | generation;
    cur_idx = asid;

    return asid;
}

void asid_initialize(void)
{
    int asid = get_cpu_asid_bits();
    if (asid < 0) {
        task_dbg("failed to get asid\n");
        return ;
    }

    asid_bits = asid;

    asid_bitmap = (asid_t *)kmalloc(NUM_USER_ASIDS);
    k_memset(asid_bitmap, 0, sizeof(*asid_bitmap));
    bitmap_zero(asid_bitmap, NUM_USER_ASIDS);

    asid_generation = ASID_FIRST_VERSION;
}

static volatile pid_t g_lasttid = -1;
static DECLARE_BITMAP(task_bitmap, CONFIG_MAX_TASKS);

/* This is the name for un-named tasks */
static const char g_noname[] = "<noname>";

void task_setup_name(struct tcb *task, const char *name)
{
    if (!name) {
        k_strncpy(task->name, g_noname, CONFIG_TASK_NAME_SIZE);
    } else {
        k_strncpy(task->name, name, CONFIG_TASK_NAME_SIZE);
    }

    task->name[CONFIG_TASK_NAME_SIZE] = '\0';
}

void task_release_tid(pid_t tid)
{
    bitmap_clear(task_bitmap, tid, 1);
    g_lasttid = tid - 1;
}

int task_assign_tid(struct tcb *task)
{
    int pid = g_lasttid + 1;

    unsigned long tid = find_next_zero_bit(task_bitmap, CONFIG_MAX_TASKS, pid);
    if (tid >= CONFIG_MAX_TASKS)
        return INVALID_TASK_ID;
    bitmap_set(task_bitmap, tid, 1);

    task->tid = tid;
    g_lasttid = tid;

    return task->tid;
}

void task_init(struct tcb *task, uint8_t type)
{
    k_memset(task, 0, sizeof(struct tcb));

    INIT_LIST_HEAD(&task->link_head);

    task->task_state = TSTATE_TASK_INACTIVE;
    task->init_priority = CONFIG_DEFAULT_TASK_PRIORITY;
    task->sched_priority = CONFIG_DEFAULT_TASK_PRIORITY;
    task->timeslice = CONFIG_RR_INTERVAL;

    task->type = type;

    context_init(&task->context, type);

    task_setup_name(task, NULL);

    INIT_LIST_HEAD(&task->mm.link_head);
}

void task_switch(struct tcb *from, struct tcb *to)
{
    asid_t asid = check_and_switch_context(to);

    task_dbg("switch to %d[%d] from %d[%d], asid = %p\n", to->tgid, to->tid, from->tgid, from->tid, asid);

    if (to->tgid != from->tgid) {
#ifdef CONFIG_HYPERVISOR_SUPPORT
        hyper_as_switch(to->addrspace, to->type, asid << (BITS_PER_LONG - asid_bits));
#else
        as_switch(to->addrspace, to->type, asid << (BITS_PER_LONG - asid_bits));
#endif
        if (b_tlb_flush) {
            b_tlb_flush = false;
            task_dbg("flush_TLB\n");
            flush_TLB();
        }
    }

#ifdef CONFIG_HYPERVISOR_SUPPORT
    if (to->vcpu.active) {
        MSR("hcr_el2", HCR_VCPU);
    } else {
        MSR("hcr_el2", HCR_NATIVE);
    }
#endif

    task_dbg("switch over\n");
}

void task_create(struct tcb *task, task_entry entry, const char *name, uint8_t priority, void *stack, uint32_t stack_size, uint8_t type, struct addrspace *as)
{
    task_init(task, type);

    task_setup_name(task, name);

    task_assign_tid(task);

    assert(priority < CONFIG_MAX_TASK_PRIORITY);
    task->init_priority = priority;
    task->sched_priority = priority;
    
    task->entry = entry;
    context_set_entry(&task->context, (unsigned long)entry);

    task->task_state = TSTATE_TASK_INACTIVE;

    task->stack = stack;
    task->stack_size = stack_size;
    context_set_stack(&task->context, stack, stack_size);

    task->addrspace = as;
}

int sys_thread_create(unsigned long entry, unsigned long stack)
{
    task_dbg("sys_thread_create\n");

    struct tcb *task = (struct tcb *)kmalloc(sizeof(struct tcb));
    k_memset(task, 0, sizeof(*task));
    struct tcb *current = this_task();
    struct addrspace *as = current->addrspace;

    task_create(task, \
                (task_entry)entry, \
                "thread1", \
                CONFIG_DEFAULT_TASK_PRIORITY, \
                (void *)stack, CONFIG_DEFAULT_TASK_STACKSIZE, TASK_TYPE_USER, as);

    task_set_tgid(task, current->tgid);

    sched_attach(task);

    return 0;
}

#ifdef CONFIG_HYPERVISOR_SUPPORT
#define VM_ENTRY    RAM_PBASE

#define VM_VBASE    RAM_PBASE
#define VM_SIZE     (8 * 1024 * 1024)

#define VM_MMIO_PBASE    MMIO_PBASE
#define VM_MMIO_VBASE    MMIO_PBASE
#define VM_MMIO_SIZE     MMIO_SIZE

#include <mmap.h>
int sys_vcpu_create(unsigned long entry, unsigned long stack, unsigned long vm_base, unsigned long vm_size)
{
    task_dbg("sys_vcpu_create\n");

    struct tcb *task = (struct tcb *)kmalloc(sizeof(struct tcb));
    k_memset(task, 0, sizeof(*task));
    struct tcb *current = this_task();
    // process
    struct addrspace *as = (struct addrspace *)kmalloc(sizeof(struct addrspace));
    k_memset(as, 0, sizeof(*as));

    task_create(task, \
                (task_entry)VM_ENTRY, \
                "vcpu1", \
                CONFIG_DEFAULT_TASK_PRIORITY, \
                (void *)stack, CONFIG_DEFAULT_TASK_STACKSIZE, TASK_TYPE_USER, as);

    // process
    task_set_tgid(task, current->tgid + 1);

    context_set_spsr(&task->context, PSTATE_VCPU);

    context_set_sp(&task->context, 0);

    task->vcpu.active = true;

    sched_attach(task);

    // process
    unsigned long vm_pbase = pgd_addressing((unsigned long *)&current->addrspace->pg_table, vm_base);
    task_dbg("vm_pbase = %p\n", vm_pbase);
    if (vm_pbase == -1) {
        PANIC();
    }

    unsigned long _vm_size = VM_SIZE;
    unsigned long _vm_vbase = VM_VBASE;
    unsigned long _vm_base = (unsigned long )kmalloc(_vm_size);
    k_memset(_vm_base, 0, _vm_size);
    unsigned long _vm_pbase = vbase_to_pbase(_vm_base);
    k_memcpy((void *)_vm_base, (void *)pbase_to_vbase(vm_pbase), vm_size);

    // vcpu remap vm ram
    task_dbg("_vm_vbase = %p, _vm_pbase = %p, _vm_size = %p, _vm_size = %p\n", _vm_vbase, _vm_pbase, _vm_size, _vm_size);
    assert(_vm_vbase && _vm_pbase && _vm_size);
    struct mem_region ram_region;
    ram_region.pbase = _vm_pbase;
    ram_region.vbase = _vm_vbase;
    ram_region.size = _vm_size;
    hyper_as_map(task->addrspace, &ram_region, PROT_READ|PROT_WRITE|PROT_EXEC, RAM_NORMAL);
    //dump_pgtable_verbose(&task->addrspace->pg_table, 0);

    // vcpu map vm dev
    struct mem_region dev_region;
    dev_region.pbase = VM_MMIO_PBASE;
    dev_region.vbase = VM_MMIO_VBASE;
    dev_region.size = VM_MMIO_SIZE;
    hyper_as_map(task->addrspace, &dev_region, PROT_READ|PROT_WRITE, RAM_DEVICE);
    //dump_pgtable_verbose(&task->addrspace->pg_table, 0);

    return 0;
}
#endif

