#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>
#include <errno.h>

#include <mutex.h>
#include <scheduler.h>
#include <task.h>

#define DEBUG	1
#define debug_printk(...) do{ if(DEBUG){ printf(__VA_ARGS__); } }while(0)

#define VERBOSE	1
#define verbose_printk(...) do{ if(VERBOSE){ printf(__VA_ARGS__); } }while(0)

static void insert_waiting_task(struct mutex *m, struct task *t)
{
	struct task *task;

	if (m->waiting) {
		list_for_every_entry(&m->waiting_tasks, task, struct task, event_node) {

#ifdef SCHEDULE_ROUND_ROBIN
			if (!list_next(&m->waiting_tasks, &task->event_node)) {
				list_add_after(&task->event_node, &t->event_node);
				break;
			}
#elif defined(SCHEDULE_PRIORITY)
			if (t->priority > task->priority)
				list_add_before(&task->event_node, &t->event_node);
#endif
		}

	} else {
		list_add_head(&m->waiting_tasks, &t->event_node);
	}


}

static void remove_waiting_task(struct mutex *mutex, struct task *t)
{
	list_delete(&t->event_node);
}

static int __mutex_lock(struct mutex *mutex)
{
	int ret = 0;
	struct task *current_task = get_current_task();

	if (mutex->lock) {
		debug_printk("mutex already locked\r\n");
		ret = -EDEADLOCK;

		if (mutex->owner) {
			debug_printk("mutex has owner: %d\r\n", mutex->owner->pid);

			current_task->state = TASK_BLOCKED;
			remove_runnable_task(current_task);

			insert_waiting_task(mutex, current_task);
			mutex->waiting++;

		} else {
			debug_printk("No owner for mutex (%p)\r\n", mutex);
		}

	} else {
		mutex->lock = 1;
		mutex->owner = current_task;
		mutex->waiting = 0;
	}

	return ret;
}

void init_mutex(struct mutex *mutex) {
	mutex->lock = 0;
	mutex->owner = NULL;
	mutex->waiting = 0;

	list_initialize(&mutex->waiting_tasks);
}

void mutex_lock(struct mutex *mutex)
{
	int ret;

	ret = __mutex_lock(mutex);
	if (ret < 0) {
		debug_printk("mutex_lock FAILED !\r\n");

	} else {
		debug_printk("mutex (%p) lock\r\n", mutex);
	}
}

void mutex_unlock(struct mutex *mutex)
{
	struct task *current_task = get_current_task();
	struct task *task;

	if (!mutex->lock)
		debug_printk("mutex already unlock\r\n");

	if (mutex->owner == current_task) {
		mutex->lock = 0;
		mutex->owner = NULL;

		if (mutex->waiting) {
			mutex->waiting--;

			if (!list_is_empty(&mutex->waiting_tasks)) {
				task = list_peek_head_type(&mutex->waiting_tasks, struct task, event_node);
				task->state = TASK_RUNNABLE;

				remove_waiting_task(mutex, task);
				insert_runnable_task(task);
			}
		}

		debug_printk("mutex (%p) unlock\r\n", mutex);

	} else {
		debug_printk("mutex cannot be unlock, task is not the owner\r\n");
	}
}
