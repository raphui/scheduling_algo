/*
 * Copyright (C) 2014  Raphaël Poggi <poggi.raph@gmail.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#include <task.h>
#include <scheduler.h>

#define DEBUG	0
#define debug_printk(...) do{ if(DEBUG){ printf(__VA_ARGS__); } }while(0)

#define VERBOSE	0
#define verbose_printk(...) do{ if(VERBOSE){ printf(__VA_ARGS__); } }while(0)

static struct task *current_task = NULL;
static struct task *previous_task = NULL;

static int task_count = 0;

#ifdef SCHEDULE_PREEMPT
#define NB_RUN_QUEUE		32
#define MAX_PRIORITIES		32
#define HIGHEST_PRIORITY	(MAX_PRIORITIES - 1)
#else
#define NB_RUN_QUEUE	1
#endif

static unsigned int run_queue_bitmap;
static struct list_node run_queue[NB_RUN_QUEUE];

static void idle_task(void)
{
	while(1)
		;
}

static void insert_in_run_queue_head(struct task *t)
{
	list_add_head(&run_queue[t->priority], &t->node);
	run_queue_bitmap |= (1 << t->priority);
}

static void insert_in_run_queue_tail(struct task *t)
{
	list_add_tail(&run_queue[t->priority], &t->node);
	run_queue_bitmap |= (1 << t->priority);
}

static void insert_task(struct task *t)
{
	struct task *task = NULL;

#ifdef SCHEDULE_ROUND_ROBIN
	insert_in_run_queue_tail(t);
	verbose_printk("inserting task %d\r\n", t->pid);
#elif defined(SCHEDULE_PRIORITY)
	task = list_peek_head_type(&run_queue[0], struct task, node);

	list_for_every_entry(&run_queue[0], task, struct task, node) {
		verbose_printk("t->priority: %d, task->priority; %d\r\n", t->priority, task->priority);
		if (t->priority > task->priority) {
			verbose_printk("inserting task %d\r\n", t->pid);
			list_add_before(&task->node, &t->node);
			break;
		}
	}
#endif
}


void task_init(void)
{
	int i;

	for (i = 0; i < NB_RUN_QUEUE; i++)
		list_initialize(&run_queue[i]);

	add_task(&idle_task, 0);
}

void add_task(void (*func)(void), unsigned int priority)
{
	struct task *task = (struct task *)malloc(sizeof(struct task));

	memset(task, 0, sizeof(struct task));

	task->state = TASK_RUNNABLE;
	task->pid = task_count;

#ifdef SCHEDULE_PRIORITY
	task->priority = priority;
#elif defined(SCHEDULE_ROUND_ROBIN)
	task->quantum = TASK_QUANTUM;
#elif defined(SCHEDULE_PREEMPT)
	task->priority = priority;
	task->quantum = TASK_QUANTUM;
#endif

	task->delay = 0;
	task->func = func;

	getcontext(&task->context);
	task->context.uc_link = 0;
	task->context.uc_stack.ss_sp = malloc(TASK_STACK_SIZE);
	task->context.uc_stack.ss_size = TASK_STACK_SIZE;
	task->context.uc_stack.ss_flags = 0;
	/* Creating task context */
	makecontext(&task->context, task->func, 0);

#ifdef SCHEDULE_PREEMPT
	insert_in_run_queue_tail(task);
#else
	insert_task(task);
#endif

	task_count++;
}

void switch_task(struct task *task)
{
	char c = 0x40;

	task->state = TASK_RUNNING;
	previous_task = current_task;
	current_task = task;

	if (task->pid != 0)
		remove_runnable_task(task);

	printf("%c(%d)\n", c + current_task->pid, current_task->pid);
}

struct task *get_previous_task(void)
{
	return previous_task;
}


struct task *get_current_task(void)
{
	return current_task;
}

struct task *find_next_task(void)
{
	struct task *task = NULL;

#ifdef SCHEDULE_PREEMPT
	unsigned int bitmap = run_queue_bitmap;
	unsigned int next_queue;

	while (bitmap)	{
		next_queue = HIGHEST_PRIORITY - __builtin_clz(bitmap) 
			- (sizeof(run_queue_bitmap) * 8 - MAX_PRIORITIES);

		list_for_every_entry(&run_queue[next_queue], task, struct task, node) {
			if (list_is_empty(&run_queue[next_queue]))
				run_queue_bitmap &= ~(1 << next_queue);

			return task;
		}

		bitmap &= ~(1 << next_queue);
	}
#elif defined(SCHEDULE_PRIORITY)
	task = list_peek_head_type(&run_queue, struct task, node);

	if (current_task && current_task->state != TASK_BLOCKED)
		if (task->priority < current_task->priority)
			task = current_task;
#elif defined(SCHEDULE_ROUND_ROBIN)
	list_for_every_entry(&run_queue[0], task, struct task, node)
		if ((task->quantum > 0) && (task->pid != 0))
			break;

	if (current_task)
		current_task->quantum = TASK_QUANTUM;

	/* Only idle task is eligible */
	if (!task) {
		task = list_peek_head_type(&run_queue[0], struct task, node);
	}
#endif

	verbose_printk("next task: %d\r\n", task->pid);

	return task;
}

void insert_runnable_task(struct task *task)
{
#ifdef SCHEDULE_PREEMPT
	if (task->quantum > 0)
		insert_in_run_queue_head(task);
	else
		insert_in_run_queue_tail(task);
#else
	insert_task(task);
#endif
	task->state = TASK_RUNNABLE;
}

void remove_runnable_task(struct task *task)
{

#ifdef SCHEDULE_ROUND_ROBIN
	current_task->quantum = TASK_QUANTUM;
#endif

	list_delete(&task->node);
}
