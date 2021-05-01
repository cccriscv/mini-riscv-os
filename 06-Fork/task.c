#include "task.h"
#include "lib.h"

uint8_t task_stack[MAX_TASK][STACK_SIZE];
struct context ctx_os;
struct task_node
{
	/* para: parent_id
	 * defaut: -1 (I'm parent)
	 */
	int parent_id;
	struct context ctx;
};
struct task_node ctx_tasks[MAX_TASK];
struct context *ctx_now;
int taskTop = 0; // total number of task

// create a new task
int task_create(void (*task)(void))
{
	if (MAX_TASK + 1 == taskTop)
		return -1;
	int i = taskTop++;
	ctx_tasks[i].ctx.ra = (reg_t)task;
	ctx_tasks[i].ctx.sp = (reg_t)&task_stack[i][STACK_SIZE - 1];
	ctx_tasks[i].parent_id = -1;
	return i;
}

// fork a child task
int task_fork(int pid)
{
	if (MAX_TASK + 1 == taskTop)
		return -1;
	// Parent process
	if (ctx_tasks[pid].parent_id == -1)
	{
		int i = taskTop++;
		ctx_tasks[i].ctx.ra = ctx_tasks[pid].ctx.ra;
		ctx_tasks[i].ctx.sp = ctx_tasks[pid].ctx.sp;
		ctx_tasks[i].parent_id = pid;
		return i;
	}
	return 0;
}

// switch to task[i]
void task_go(int i)
{
	ctx_now = &ctx_tasks[i].ctx;
	sys_switch(&ctx_os, &ctx_tasks[i].ctx);
}

// switch back to os
void task_os()
{
	struct context *ctx = ctx_now;
	ctx_now = &ctx_os;
	sys_switch(ctx, &ctx_os);
}
