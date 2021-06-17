#include "os.h"

int shared_var = 500;

void user_task0(void)
{
	lib_puts("Task0: Created!\n");
	while (1)
	{
		lib_puts("Task0: Running...\n");
		lib_delay(1000);
	}
}

void user_task1(void)
{
	lib_puts("Task1: Created!\n");
	while (1)
	{
		lib_puts("Task1: Running...\n");
		lib_delay(1000);
	}
}

void user_task2(void)
{
	lib_puts("Task2: Created!\n");
	while (1)
	{
		for (int i = 0; i < 50; i++)
		{
			spinlock_lock();
			shared_var++;
			spinlock_unlock();
		}
		lib_delay(6000);
		lib_printf("The value of shared_var is: %d \n", shared_var);
	}
}

void user_init()
{
	task_create(&user_task0);
	task_create(&user_task1);
	task_create(&user_task2);
}
