#include "task.h"
#include "lib.h"
#include "os.h"
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
	if (MAX_TASK == taskTop)
		return -1;
	ctx_tasks[taskTop].ctx.ra = (reg_t)task;
	ctx_tasks[taskTop].ctx.sp = (reg_t)&task_stack[taskTop][STACK_SIZE - 1];
	ctx_tasks[taskTop].parent_id = -1;
	return taskTop++;
}

// fork a child process
int task_fork(int pid)
{
	if (MAX_TASK == taskTop)
		return -1;
	// Parent process
	if (ctx_tasks[pid].parent_id == -1)
	{
		ctx_tasks[taskTop].ctx.ra = ctx_tasks[pid].ctx.ra;
		ctx_tasks[taskTop].ctx.sp = ctx_tasks[pid].ctx.sp;
		ctx_tasks[taskTop].parent_id = pid;
		return taskTop++;
	}
	return 0;
}

// Kill the child process
void task_killer()
{
	for (int i = 0; i <= 1; i++)
	{
		ctx_tasks[taskTop].ctx.ra = 0;
		ctx_tasks[taskTop].ctx.sp = 0;
		ctx_tasks[taskTop].parent_id = 0;
		taskTop--;
		minus_current_task();
	}
	os_kernel();
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
