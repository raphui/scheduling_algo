/*
 * Copyright (C) 2015  Raphaël Poggi <poggi.raph@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <semaphore.h>
#include <task.h>
#include <scheduler.h>
#include <stdio.h>

#define DEBUG	1
#define debug_printk(...) do{ if(DEBUG){ printf(__VA_ARGS__); } }while(0)

#define VERBOSE	0
#define verbose_printk(...) do{ if(VERBOSE){ printf(__VA_ARGS__); } }while(0)

static void insert_waiting_task(struct semaphore *sem, struct task *t)
{
	struct task *task;

	if (sem->waiting) {
		list_for_every_entry(&sem->waiting_tasks, task, struct task, event_node) {
#ifdef SCHEDULE_ROUND_ROBIN
			if (!list_next(&sem->waiting_tasks, &task->event_node)) {
				list_add_after(&task->event_node, &t->event_node);
				break;
			}
#elif defined(SCHEDULE_PRIORITY)
			if (t->priority > task->priority)
				list_add_before(&task->event_node, &t->event_node);

#endif
		}

	} else {
		list_add_head(&sem->waiting_tasks, &t->event_node);
	}


}

static void remove_waiting_task(struct semaphore *sem, struct task *t)
{
	list_delete(&t->event_node);
}

static void flush_waiting_task(struct semaphore *sem)
{
	struct task *task;

	list_for_every_entry(&sem->waiting_tasks, task, struct task, event_node)
		list_clear_node(&task->event_node);
}

void init_semaphore(struct semaphore *sem, unsigned int value)
{
	sem->value = value;
	sem->count = 0;
	sem->waiting = 0;

	list_initialize(&sem->waiting_tasks);
}

void sem_wait(struct semaphore *sem)
{
	struct task *current_task;

	if (sem->count < sem->value) {
		debug_printk("sem (%p) got\r\n", sem);
		sem->count++;
	} else {
		debug_printk("unable to got sem (%p)\r\n", sem);

		current_task = get_current_task();
		current_task->state = TASK_BLOCKED;

		insert_waiting_task(sem, current_task);

		sem->waiting++;

		schedule_task(NULL);

	}
}

void sem_post(struct semaphore *sem)
{
	struct task *task;

	if (sem->waiting) {
		debug_printk("%d tasks are waiting for sem (%p)\r\n", sem->waiting, sem);

		sem->waiting--;
		sem->count--;

		if (!list_is_empty(&sem->waiting_tasks)) {
			task = list_peek_head_type(&sem->waiting_tasks, struct task, event_node);
			task->state = TASK_RUNNABLE;

			remove_waiting_task(sem, task);
			insert_runnable_task(task);
			flush_waiting_task(sem);
			sem->waiting = 0;
		}

	} else {
		if (sem->count == 0)
			debug_printk("all sem (%p) token has been post\r\n", sem);
		else
			sem->count--;

		debug_printk("sem (%p) post\r\n", sem);
	}
}
