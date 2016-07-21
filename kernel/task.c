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
#include <signal.h>
#include <ucontext.h>

#include <scheduler.h>

static struct task *previous_task = NULL;
static struct task *current_task = NULL;
static int task_count = 0;

#ifndef SCHEDULE_PREEMPT
LIST_HEAD(, task) runnable_tasks = LIST_HEAD_INITIALIZER(runnable_tasks);
#else

#define MAX_PRIORITY	32

static unsigned int run_queue_bitmap;

static struct list_node run_queue[MAX_PRIORITY];
#endif

static void idle_task(void)
{
	while(1)
		;
}

#ifdef SCHEDULE_PREEMPT
static void insert_in_run_queue_head(struct task *t)
{
    list_add_head(&run_queue[t->priority], &t->next);
    run_queue_bitmap |= (1 << t->priority);
}

static void insert_in_run_queue_tail(struct task *t)
{
    list_add_tail(&run_queue[t->priority], &t->next);
    run_queue_bitmap |= (1 << t->priority);
}
#else
static void insert_task(struct task *t)
{
	struct task *task;

	task = LIST_FIRST(&runnable_tasks);

	if (!task_count) {
//		printf("runnable list is empty\r\n");
		LIST_INSERT_HEAD(&runnable_tasks, task, next);
		return;
	}

	LIST_FOREACH(task, &runnable_tasks, next) {
#ifdef SCHEDULE_ROUND_ROBIN
		if (!LIST_NEXT(task, next)) {
//			printf("inserting task %d\r\n", t->pid);
			LIST_INSERT_AFTER(task, t, next);
			break;
		}
#elif defined(SCHEDULE_PRIORITY)
//		printf("t->priority: %d, task->priority; %d\r\n", t->priority, task->priority);
		if (t->priority > task->priority) {
//			printf("inserting task %d\r\n", t->pid);
			LIST_INSERT_BEFORE(task, t, next);
			break;
		}
#endif
	}
}
#endif

void task_init(void)
{
#ifndef SCHEDULE_PREEMPT
	LIST_INIT(&runnable_tasks);
#endif
	add_task(&idle_task, 0);
}

void add_task(void (*func)(void), unsigned int priority)
{
	struct task *task = (struct task *)malloc(sizeof(struct task));
	task->state = TASK_RUNNABLE;
	task->pid = task_count;

#ifdef SCHEDULE_PREEMPT
	task->priority = priority;
	task->quantum = TASK_QUANTUM;
#else
#ifdef SCHEDULE_PRIORITY
	task->priority = priority;
#elif defined(SCHEDULE_ROUND_ROBIN)
	task->quantum = TASK_QUANTUM;
#endif
#endif

	task->delay = 0;
	task->func = func;

	getcontext(&task->context);
	task->context.uc_link = 0;
	task->context.uc_stack.ss_sp = malloc(TASK_STACK_SIZE);
        task->context.uc_stack.ss_size = TASK_STACK_SIZE;
        task->context.uc_stack.ss_flags = 0;

	makecontext(&task->context, task->func, 0);

#ifdef SCHEDULE_PREEMPT
	insert_in_run_queue_tail(task);
#else
	if (task_count)
		insert_task(task);
	else
		LIST_INSERT_HEAD(&runnable_tasks, task, next);
#endif

	task_count++;

	printf("creating task n°%d [%#x] with func %#x\n", task->pid, task, task->func);
}

void switch_task(struct task *task)
{
	if (current_task && (current_task->state != TASK_BLOCKED)) {
#ifndef SCHEDULE_PREEMPT
		insert_task(current_task);
#endif
	}

	task->state = TASK_RUNNING;
	previous_task = current_task;
	current_task = task;

#ifdef SCHEDULE_PREEMPT

#else
	if (task->pid != 0)
		remove_runnable_task(task);
#endif

	char c = 0x40;
	printf("%c ", c + current_task->pid);
}

struct task *get_current_task(void)
{
	return current_task;
}

struct task *get_previous_task(void)
{
	return previous_task;
}

struct task *find_next_task(void)
{
	struct task *task = NULL;

#ifdef SCHEDULE_PRIORITY
	task = LIST_FIRST(&runnable_tasks);

	if (current_task && current_task->state != TASK_BLOCKED)
		if (task->priority < current_task->priority)
			task = current_task;
#elif defined(SCHEDULE_ROUND_ROBIN)
	LIST_FOREACH(task, &runnable_tasks, next)
		if ((task->quantum > 0) && (task->pid != 0))
			break;

	if (current_task)
		current_task->quantum = TASK_QUANTUM;

	/* Only idle task is eligible */
	if (!task)
		task = LIST_FIRST(&runnable_tasks);
#endif

	printf("next task: %d\r\n", task->pid);

	return task;
}

void insert_runnable_task(struct task *task)
{
	insert_task(task);
	task->state = TASK_RUNNABLE;
}

void remove_runnable_task(struct task *task)
{
#ifdef SCHEDULE_ROUND_ROBIN
	current_task->quantum = TASK_QUANTUM;
#endif
	LIST_REMOVE(task, next);
}
