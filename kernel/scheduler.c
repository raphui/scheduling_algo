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
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#include <task.h>
#include <scheduler.h>

#define DEBUG	0
#define debug_printk(...) do{ if(DEBUG){ printf(__VA_ARGS__); } }while(0)

#define VERBOSE	0
#define verbose_printk(...) do{ if(VERBOSE){ printf(__VA_ARGS__); } }while(0)

int task_switching = 0;
unsigned int system_tick = 0;

void tick_handler(int signum)
{
	schedule_task(NULL);
}

int schedule_init(void)
{
	int ret = 0;

	task_init();

	return ret;
}

void start_schedule(void)
{
	struct itimerval timer;

	memset(&timer, 0, sizeof(struct itimerval));
	signal(SIGALRM, &tick_handler);

	timer.it_interval.tv_usec = 100000;
	timer.it_value.tv_usec = 100000;
	setitimer(ITIMER_REAL, &timer, NULL);
}

void schedule(void)
{
	struct task *t;

	t = find_next_task();
	switch_task(t);
	task_switching = 1;
}

void schedule_task(struct task *task)
{
	struct task *t;
	struct task *previous_task;
	struct task *current_task;

#if defined(SCHEDULE_ROUND_ROBIN) || defined(SCHEDULE_PREEMPT)
	t = get_current_task();
	if (t) {
		t->quantum--;
		debug_printk("task [%p] quantum at %d\n", t, t->quantum);
	}
#endif

	if (task) {
		current_task = get_current_task();
		if (current_task && (current_task->state != TASK_BLOCKED)) {
			insert_runnable_task(current_task);
		}

		switch_task(task);
		previous_task = get_previous_task();
		current_task = get_current_task();

		if (previous_task)
			swapcontext(&previous_task->context, &current_task->context);
		else
			setcontext(&current_task->context);
	} else {
#ifdef SCHEDULE_PREEMPT
		t = find_next_task();
		switch_task(t);
		previous_task = get_previous_task();
		current_task = get_current_task();

		if (previous_task)
			swapcontext(&previous_task->context, &current_task->context);
		else
			setcontext(&current_task->context);
			
#elif defined(SCHEDULE_ROUND_ROBIN)
		if (!t || !t->quantum || (t->state == TASK_BLOCKED)) {
			t = find_next_task();
			switch_task(t);
			previous_task = get_previous_task();
			current_task = get_current_task();

			if (previous_task)
				swapcontext(&previous_task->context, &current_task->context);
			else
				setcontext(&current_task->context);
		}
#elif defined(SCHEDULE_PRIORITY)
		t = find_next_task();
		switch_task(t);
		previous_task = get_previous_task();
		current_task = get_current_task();

		if (previous_task)
			swapcontext(&previous_task->context, &current_task->context);
		else
			setcontext(&current_task->context);
#endif
	}
}
