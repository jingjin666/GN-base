#ifndef __SCHED_SCHEDULER_H
#define __SCHED_SCHEDULER_H

#include <uapi/list.h>

#include <task.h>

typedef struct sched_queue
{
    struct list_head tasks_head;
} sched_queue_t;

typedef struct tid_hash
{
	struct tcb *task;
	pid_t  tid;
} tid_hash_t;

extern struct tcb *g_current_task;
extern struct sched_queue g_readytorun[CONFIG_MAX_TASK_PRIORITY];

#define this_task() g_current_task

void sched_init(struct tcb *idle);
void sched_attach(struct tcb *task);
void sched_detach(struct tcb *task);
void sched_unblock(struct tcb *task);
void sched_block(struct tcb *task);
void sched_sleep(struct tcb *task);
void sched_wake(struct tcb *task);
void sched_pending_check(void);
void schedule(void);
#endif
