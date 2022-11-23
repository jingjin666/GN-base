#ifndef __SCHED_TASK_H
#define __SCHED_TASK_H

#include <chinos/config.h>

#include <k_time.h>
#include <uapi/list.h>
#include <addrspace.h>
#include <context.h>

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

typedef struct tcb
{
    struct list_head link_head;

    tid_t           tid;                        /* This is the task ID of the task */
    tid_t           tgid;                       /* This is the task group ID of the task */
    uint8_t         sched_priority;             /* Current priority of the task */
    uint8_t         init_priority;              /* Initial priority of the task */
    task_entry      entry;                      /* Entry Point into the task */
    task_state_e    task_state;                 /* Current state of the task */

#if CONFIG_TASK_NAME_SIZE > 0
    char name[CONFIG_TASK_NAME_SIZE + 1];  /* Task name (with NUL terminator  */
#endif

    void           *stack;
    uint32_t        stack_size;

    time64_t        readytime;
    uint32_t        timeslice;

    context_t       context;

    addrspace_t    *addrspace;
} tcb_t;

void task_setup_name(tcb_t *task, const char *name);
void task_release_tid(tid_t tid);
int  task_assign_tid(tcb_t *task);
void task_init(tcb_t *task, uint8_t type);
void task_switch(tcb_t *from, tcb_t *to);
void task_create(struct tcb *task, task_entry entry, const char *name, uint8_t priority, void *stack, uint32_t stack_size, uint8_t type, struct addrspace *as);
#endif
