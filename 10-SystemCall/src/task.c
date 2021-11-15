#include "task.h"
#include "lib.h"

uint8_t task_stack[MAX_TASK][STACK_SIZE];
struct context ctx_os;
struct context ctx_tasks[MAX_TASK];
struct context *ctx_now;
int taskTop = 0; // total number of task

// create a new task
int task_create(void (*task)(void))
{
	int i = taskTop++;
	ctx_tasks[i].ra = (reg_t)task;
	ctx_tasks[i].sp = (reg_t)&task_stack[i][STACK_SIZE - 1];
	return i;
}

// switch to task[i]
void task_go(int i)
{
	ctx_now = &ctx_tasks[i];
	sys_switch(&ctx_os, &ctx_tasks[i]);
}

// switch back to os
void task_os()
{
	struct context *ctx = ctx_now;
	ctx_now = &ctx_os;
	sys_switch(ctx, &ctx_os);
}
