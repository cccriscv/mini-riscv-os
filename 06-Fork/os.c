#include "os.h"

void os_kernel()
{
	task_os();
}

void os_start()
{
	lib_puts("OS start\n");
	user_init();
	timer_init(); // start timer interrupt ...
}

int current_task = 0;

int get_current_task()
{
	return current_task;
}

void minus_current_task()
{
	current_task--;
}

int os_main(void)
{
	os_start();

	while (1)
	{
		lib_puts("OS: Activate next task\n");
		task_go(current_task);
		lib_puts("OS: Back to OS\n");
		current_task = (current_task + 1) % taskTop; // Round Robin Scheduling
		lib_puts("\n");
	}
	return 0;
}
