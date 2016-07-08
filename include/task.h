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

#ifndef TASK_H
#define TASK_H

#include <stddef.h>
#include <ucontext.h>

#include <list.h>

#define TASK_RUNNING		0
#define TASK_RUNNABLE		1
#define TASK_STOPPED		2
#define TASK_INTERRUPTIBLE	3
#define TASK_BLOCKED		4

#define TASK_QUANTUM		150
#define TASK_STACK_SIZE		64 * 1024

struct task
{
	unsigned int state;
	unsigned int pid;
	unsigned int priority;
	unsigned int quantum;
	unsigned int start_stack;
	unsigned int delay;
	ucontext_t context;
	void (*func)(void);
	LIST_ENTRY(task) next;
};

void task_init(void);
void add_task(void (*func)(void), unsigned int priority);
void first_switch_task(void);
void switch_task(struct task *task);
struct task *get_current_task(void);
struct task *find_next_task(void);
void insert_runnable_task(struct task *task);
void remove_runnable_task(struct task *task);

#endif /* TASK_H */
