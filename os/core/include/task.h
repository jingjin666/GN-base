#ifndef __SCHED_TASK_H
#define __SCHED_TASK_H

#include <chinos/config.h>

#include <k_time.h>
#include <uapi/list.h>
#include <addrspace.h>
#include <context.h>
#ifdef CONFIG_HYPERVISOR_SUPPORT
#include <vcpu.h>
#endif

#define TASK_TYPE_USER      0x00
#define TASK_TYPE_KERNEL    0xff

typedef int (*task_entry)(int argc, char *argv[]);

typedef enum task_state
{
    TSTATE_TASK_INVALID    = 0, /* INVALID      - The TCB is uninitialized */
    TSTATE_TASK_INACTIVE,       /* INACTIVE     - Initialized but not yet activated */
    TSTATE_TASK_READYTORUN,     /* READY-TO-RUN - But not running */
    TSTATE_TASK_RUNNING,        /* READY_TO_RUN - And running */
    TSTATE_TASK_SLEEPING,       /* READY_TO_RUN - But sleeping */
    TSTATE_TASK_PENDING,        /* READY_TO_RUN - But blocked */
    TSTATE_TASK_SUSPEND,        /* READY_TO_RUN - But suspend util resume */
    NUM_TASK_STATES             /* Must be last */
} task_state_e;

typedef struct mm_area
{
    struct list_head link_head;         /* 线性区链表头 */
    unsigned long mmap_base;		    /* 标识第一个分配的匿名线性区或文件内存映射的线性地址 */

    /*
     * start_brk-   堆的超始地址
     * brk-         堆的当前最后地址
     * start_stack- 用户态堆栈的起始地址
     * end_stack-   用户态堆栈的结束地址
     */
    unsigned long start_brk, brk, start_stack, end_stack;

    /*
	 * start_code-  可执行代码的起始地址
	 * end_code-    可执行代码的最后地址
	 * start_data-  已初始化数据的起始地址
	 * end_data--   已初始化数据的结束地址
	 */
    unsigned long start_code, end_code, start_data, end_data;

    /*
	 * start_bss-   未初始化/初始化0数据的起始地址
	 * end_bss-     未初始化/初始化0数据的结束地址
	 */
    unsigned long start_bss, end_bss;
} mm_area_t;

#ifdef CONFIG_HYPERVISOR_SUPPORT
typedef struct vcpu
{
    uint8_t active;
    struct tcb *task;
    struct vcpu_gic vgic;
    uint64_t regs[VCPU_REG_NUM];
} vcpu_t;
#endif

typedef struct tcb
{
    struct list_head link_head;

    pid_t           tid;                        /* This is the task ID of the task */
    pid_t           tgid;                       /* This is the task group ID of the task */
    struct tcb     *group_leader;               /* This is the group leader of the task group */ 
    uint8_t         sched_priority;             /* Current priority of the task */
    uint8_t         init_priority;              /* Initial priority of the task */
    task_entry      entry;                      /* Entry Point into the task */
    task_state_e    task_state;                 /* Current state of the task */

#if CONFIG_TASK_NAME_SIZE > 0
    char name[CONFIG_TASK_NAME_SIZE + 1];  /* Task name (with NUL terminator  */
#endif

    void           *stack;
    uint32_t        stack_size;

    ticks_t         readytime;
    uint32_t        timeslice;
    uint8_t         type;

    context_t       context;

#ifdef CONFIG_HYPERVISOR_SUPPORT
    vcpu_t          vcpu;
#endif

    addrspace_t    *addrspace;

    mm_area_t       mm;

    asid_t          asid;
} tcb_t;

typedef struct vm_area
{
    struct list_head link_head;

    mm_area_t *vm_mm;

	unsigned long vm_start;		/* 起始虚拟地址 */
	unsigned long vm_end;		/* 结束虚拟地址 */

	unsigned long vm_pgoff;		/* 起始虚拟地址对应的文件偏移 */
	int vm_file_fd;		        /* 虚拟地址对应的映射文件 */

    unsigned long vm_prot;		/* 地址空间属性(R/W/E) */
	unsigned long vm_flags;		/* 地址空间类型(REL/DYN/EXEC) */

	char vm_name[64];
} vm_area_t;

static inline void task_set_tid(struct tcb *task, pid_t tid)
{
    task->tid = tid;
}

static inline void task_set_tgid(struct tcb *task, pid_t tgid)
{
    task->tgid = tgid;
}

void asid_initialize(void);
void task_setup_name(struct tcb *task, const char *name);
void task_release_tid(pid_t tid);
int  task_assign_tid(struct tcb *task);
void task_init(struct tcb *task, uint8_t type);
void task_switch(struct tcb *from, struct tcb *to);
void task_create(struct tcb *task, task_entry entry, const char *name, uint8_t priority, void *stack, uint32_t stack_size, uint8_t type, struct addrspace *as);
int sys_thread_create(unsigned long entry, unsigned long stack);
int sys_vcpu_create(unsigned long entry, unsigned long stack, unsigned long vm_base, unsigned long vm_size);
#endif
