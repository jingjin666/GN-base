#include <chinos/config.h>

#include <kernel.h>
#include <k_stddef.h>
#include <k_stdbool.h>
#include <k_assert.h>
#include <k_debug.h>
#include <task.h>
#include <generic_timer.h>
#include <timer.h>

#include "scheduler.h"

//#define KERNEL_SCHED_SCHEDULER_DEBUG
#ifdef KERNEL_SCHED_SCHEDULER_DEBUG
#define sched_dbg kdbg
#define sched_err kerr
#define sched_warn kwarn
#define sched_info kinfo
#else
#define sched_dbg(fmt, ...)
#define sched_err(fmt, ...)
#define sched_warn(fmt, ...)
#define sched_info(fmt, ...)
#endif

/* 当前运行的任务 */
struct tcb *g_current_task = NULL;

/* 任务就绪队列数组，每个优先级都对应一个队列 */
struct sched_queue g_readytorun[CONFIG_MAX_TASK_PRIORITY];

/* 优先级位图 */
#define MAX_PRIORITY_BITMAP_INDEX (CONFIG_MAX_TASK_PRIORITY / 8)
uint8_t g_priority_bitmap[MAX_PRIORITY_BITMAP_INDEX];
uint8_t g_priority_group;

/* 任务阻塞队列 */
struct sched_queue g_pendingtasks;

/* 任务优先级查找表-优先级值最低位为1所在的位数 */
uint8_t const g_priority_map[256] = { 
    0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0x00 to 0x0F                             */ 
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0x10 to 0x1F                             */ 
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0x20 to 0x2F                             */ 
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0x30 to 0x3F                             */ 
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0x40 to 0x4F                             */ 
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0x50 to 0x5F                             */ 
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0x60 to 0x6F                             */ 
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0x70 to 0x7F                             */ 
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0x80 to 0x8F                             */ 
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0x90 to 0x9F                             */ 
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0xA0 to 0xAF                             */ 
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0xB0 to 0xBF                             */ 
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0xC0 to 0xCF                             */ 
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0xD0 to 0xDF                             */ 
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,       /* 0xE0 to 0xEF                             */ 
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0        /* 0xF0 to 0xFF                             */ 
};

static void dump_queue(struct list_head *queue)
{
    struct tcb *task = NULL;
    list_for_each_entry(task, queue, link_head)
    {
        sched_dbg("dump %s\n", task->name);
    }
}

void sched_init(struct tcb *idle_task)
{
    assert(idle_task != NULL);

    for (int i = 0; i < CONFIG_MAX_TASK_PRIORITY; i++) {
        INIT_LIST_HEAD(&g_readytorun[i].tasks_head);
    }

    INIT_LIST_HEAD(&g_pendingtasks.tasks_head);

    sched_attach(idle_task);

    g_current_task = idle_task;
}

void sched_attach(struct tcb *task)
{
    assert(task != NULL);

    task->task_state = TSTATE_TASK_READYTORUN;

    list_add_tail(&task->link_head, &g_readytorun[task->sched_priority].tasks_head);

    int index = task->sched_priority / 8;
    int remain = task->sched_priority % 8;
    if (index > MAX_PRIORITY_BITMAP_INDEX) {
        sched_dbg("task priority[%d] invalid\n", task->sched_priority);
        return;
    }

    g_priority_group |= bit(index);
    g_priority_bitmap[index] |= bit(remain);
}

void sched_detach(struct tcb *task)
{
    assert(task != NULL);

    task->task_state = TSTATE_TASK_INACTIVE;

	task_release_tid(task->tid);

    if (!list_empty(&g_readytorun[task->sched_priority].tasks_head)) {
        list_del(&task->link_head);
    }

    int index = task->sched_priority / 8;
    int remain = task->sched_priority % 8;
    if (index > MAX_PRIORITY_BITMAP_INDEX) {
        sched_dbg("task priority[%d] invalid\n", task->sched_priority);
        return;
    }

    if (list_empty(&g_readytorun[task->sched_priority].tasks_head)) {
        if ((g_priority_bitmap[index] &= (~ bit(remain))) == 0) {
            // 当此优先级所在组全部清空后，才清空优先级分组标识
            g_priority_group &= (~ bit(index));
        }
    }
}

void sched_unblock(struct tcb *task)
{
    assert(task != NULL);

    task->task_state = TSTATE_TASK_READYTORUN;
    list_add(&task->link_head, &g_readytorun[task->sched_priority].tasks_head);

    int index = task->sched_priority / 8;
    int remain = task->sched_priority % 8;
    if (index > MAX_PRIORITY_BITMAP_INDEX) {
        sched_dbg("task priority[%d] invalid\n", task->sched_priority);
        return;
    }

    g_priority_group |= bit(index);
    g_priority_bitmap[index] |= bit(remain);
}

void sched_block(struct tcb *task)
{
    assert(task != NULL);

    list_del_init(&task->link_head);
    task->task_state = TSTATE_TASK_PENDING;

    int index = task->sched_priority / 8;
    int remain = task->sched_priority % 8;
    if (index > MAX_PRIORITY_BITMAP_INDEX) {
        sched_dbg("task priority[%d] invalid\n", task->sched_priority);
        return;
    }

    if (list_empty(&g_readytorun[task->sched_priority].tasks_head)) {
        if ((g_priority_bitmap[index] &= (~ bit(remain))) == 0) {
            // 当此优先级所在组全部清空后，才清空优先级分组标识
            g_priority_group &= (~ bit(index));
        }
    }
}

void sched_sleep(struct tcb *task)
{
    assert(task != NULL);

    sched_dbg("schedule_sleep %s\n", task->name);

    list_del_init(&task->link_head);
    task->task_state = TSTATE_TASK_SLEEPING;

    int index = task->sched_priority / 8;
    int remain = task->sched_priority % 8;
    if (index > MAX_PRIORITY_BITMAP_INDEX) {
        sched_dbg("task priority[%d] invalid\n", task->sched_priority);
        return;
    }

    if (list_empty(&g_readytorun[task->sched_priority].tasks_head)) {
        if ((g_priority_bitmap[index] &= (~ bit(remain))) == 0) {
            // 当此优先级所在组全部清空后，才清空优先级分组标识
            g_priority_group &= (~ bit(index));
        }
    }

    list_add_tail(&task->link_head, &g_pendingtasks.tasks_head);

    sched_dbg("sleep dump pend\n");
    dump_queue(&g_pendingtasks.tasks_head);
}

void sched_wake(struct tcb *task)
{
    list_del_init(&task->link_head);

    int index = task->sched_priority / 8;
    int remain = task->sched_priority % 8;
    if (index > MAX_PRIORITY_BITMAP_INDEX) {
        sched_dbg("task priority[%d] invalid\n", task->sched_priority);
        return;
    }

    g_priority_group |= bit(index);
    g_priority_bitmap[index] |= bit(remain);
}

void sched_pending_check(void)
{
    struct tcb *pend_task = NULL;
    struct tcb *pend_task_tmp = NULL;
    sched_dbg("schedule_pending_check\n");
    list_for_each_entry_safe(pend_task, pend_task_tmp, &g_pendingtasks.tasks_head, link_head)
    {
        int64_t remain_ticks = pend_task->readytime - get_current_time();
        sched_dbg("next_task name is %s, tid = %d, readytime = %lld, remain_ticks = %lld\n", pend_task->name, pend_task->tid, pend_task->readytime, remain_ticks);
        if (remain_ticks < 0) {
            sched_dbg("wake pend task %s\n", pend_task->name);
            sched_wake(pend_task);
            sched_attach(pend_task);
        }
    }
}

void schedule(void)
{
    struct tcb *cur_task = this_task();
    sched_dbg("cur_task name is %s\n", cur_task->name);

    sched_pending_check();

    uint8_t row = g_priority_map[g_priority_group];
    uint8_t re = g_priority_bitmap[row];
    uint8_t col = g_priority_map[re];
    uint8_t sched_priority = row * 8 + col;
    sched_dbg("sched_priority is %d\n", sched_priority);

    struct tcb *task = NULL;
    struct tcb *task_tmp = NULL;
    sched_queue_t *ready_queue = &g_readytorun[sched_priority];

    sched_dbg("dump1 ready\n");
    dump_queue(&ready_queue->tasks_head);

    sched_dbg("dump1 pend\n");
    dump_queue(&g_pendingtasks.tasks_head);

    /* 找到需要调度的任务 */
    list_for_each_entry_safe(task, task_tmp, &ready_queue->tasks_head, link_head)
    {
        sched_dbg("current_task is %s, task is %s\n", cur_task->name, task->name);
        if (cur_task != task)
        {
            sched_dbg("schedule %s\n", task->name);
            /*
             * 如果要执行的任务不是当前任务则进行切换
             * 如果要执行的任务是当前任务，则不进行切换
             * 根据优先级抢占式调度，当前优先级高的任务不主动放弃，低优先级任务永远不能得到执行
             */
            task->task_state = TSTATE_TASK_RUNNING;
            g_current_task = task;

            /* 把当前任务从ready队列删除，并添加到pending队尾 */
            list_del(&task->link_head);
            list_add_tail(&task->link_head, &g_pendingtasks.tasks_head);

            sched_dbg("task_switch %s\n", task->name);
            task_switch(cur_task, task);

            sched_dbg("schedule over %s\n", task->name);
            break;
        }
    }

    sched_dbg("dump2 ready\n");
    dump_queue(&ready_queue->tasks_head);

    sched_dbg("dump2 pend\n");
    dump_queue(&g_pendingtasks.tasks_head);
}
