#include <chinos/config.h>

#include <kernel.h>
#include <k_stdint.h>
#include <k_stdlib.h>
#include <k_stdbool.h>
#include <k_string.h>
#include <k_limits.h>
#include <k_assert.h>
#include <k_debug.h>
#include <scheduler.h>
#include <uapi/errors.h>
#include <addrspace.h>
#include <context.h>

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

#define INVALID_PROCESS_ID       (tid_t)-1
#define MAX_TASKS_MASK           (CONFIG_MAX_TASKS-1)
#define PIDHASH(tid)             ((tid) & MAX_TASKS_MASK)

volatile tid_t g_lasttid = 0;
struct tid_hash g_tidhash[CONFIG_MAX_TASKS] = {0};

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

void task_release_tid(tid_t tid)
{
	int hash_ndx = PIDHASH(tid);
	
	g_tidhash[hash_ndx].task  = NULL;
  	g_tidhash[hash_ndx].tid   = INVALID_PROCESS_ID;
}

int task_assign_tid(struct tcb *task)
{
	tid_t next_tid;
	int   hash_ndx;
	int   tries;

	for (tries = 0; tries < CONFIG_MAX_TASKS; tries++)
    {
		next_tid = ++g_lasttid;
		if (next_tid <= 0)
		{
			g_lasttid = 1;
			next_tid  = 1;
		}

		hash_ndx = PIDHASH(next_tid);
		if (!g_tidhash[hash_ndx].task) {
			task_dbg("%s %d\n", task->name, next_tid);
			g_tidhash[hash_ndx].task  = task;
			g_tidhash[hash_ndx].tid	  = next_tid;

			task->tid = next_tid;
			return OK;
		}
	}

	return -ESRCH;
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
    as_switch(to->addrspace, to->type);
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
